#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <pthread.h>

#include <sys/select.h>


#include <sys/ipc.h>
#include <sys/shm.h>

sqlite3 *db;
//====================================
int db_init(const char * name){

    int rc = sqlite3_open(name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", 
        sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    return 1;
}
void db_close(void){
    sqlite3_close(db);
}
//====================================
int get_shmem_size(){
//    char *err_msg = 0;
    
    sqlite3_stmt *res;
    unsigned int uid;
    unsigned int max_offset, var_size;

        
    int rc = sqlite3_prepare_v2(db, "SELECT max(value) FROM variables_0 WHERE name = 'c_offset'", -1, &res, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }    
  
    int step = sqlite3_step(res);

    if (step == SQLITE_ROW) {
        const unsigned char * tmp = sqlite3_column_text(res, 0);
        max_offset = atoi((const char *)tmp);
        sqlite3_reset(res);
        rc = sqlite3_prepare_v2(db,"select uid from variables_0 where name='c_offset' and value=?",-1,&res,0);
        if(rc == SQLITE_OK) {
            rc = sqlite3_bind_int(res, 1, max_offset);
            step = sqlite3_step(res);
            if (step == SQLITE_ROW) {
                uid = sqlite3_column_int(res,0);
                fprintf(stderr, "%d\n", uid);
                sqlite3_reset(res);
                rc = sqlite3_prepare_v2(db,"select value from variables_0 where uid=? and name='size'",-1,&res,0);
                if (rc == SQLITE_OK){
                    rc = sqlite3_bind_int(res, 1, uid);
                    step = sqlite3_step(res);
                    if (step == SQLITE_ROW) {
                        var_size = sqlite3_column_int(res,0);
                        fprintf(stderr, "%d\n", var_size);
                        sqlite3_reset(res);
                    }
                }
            }
        }
    } 

    sqlite3_finalize(res);

    return max_offset + var_size;
}

//==========================================

int id;
int shmkey;
char *base_adr;
char *user_adr;
unsigned long size;
pthread_mutex_t *mutex;

int status;
enum SharedMemoryEnum
{
    OK = 0,
    ERROR_FILE,
    ERROR_SHMGET,
    ERROR_SHMAT,
    ERROR_SHMCTL
};
char *name;

//==========================================
int rlwthread_sleep(long msec)
{
  fd_set wset,rset,eset;
  struct timeval timeout;

  FD_ZERO(&rset);
  FD_ZERO(&wset);
  FD_ZERO(&eset);
  timeout.tv_sec  = msec / 1000;
  timeout.tv_usec = (msec % 1000) * 1000;
  select(1,&rset,&wset,&eset,&timeout);
  return 0;

}

static void myinit(pthread_mutex_t *mutex)
{
  int *cnt = (int *) mutex;
  *cnt = 0;
}

static void mylock(pthread_mutex_t *mutex, int increment)
{
  int *cnt = (int *) mutex;
  while(1)
  {
retry:
    if(*cnt == 0)
    { // try to lock the counter
      (*cnt) += increment;
      if(*cnt > 1)
      {
        (*cnt) -= increment;
        goto retry; // another process also wanted to lock the counter
      }
      return;       // now we can do it
    }
    rlwthread_sleep(1);
  }
}

static void myunlock(pthread_mutex_t *mutex)
{
  int *cnt = (int *) mutex;
  if(*cnt > 0) (*cnt)--;
}

//==================================
int shm_write(unsigned long offset, const void *buf, int len)
{
  void *ptr;
  if(status != OK)      return -1;
  if(len <= 0)          return -1;
  if(offset+len > size) return -1;
  ptr = user_adr + offset;

  mylock(mutex,1);
  memcpy(ptr,buf,len);
  myunlock(mutex);
  return len;
}

int shm_read(unsigned long offset, void *buf, int len)
{
  void *ptr;
  if(status != OK)      return -1;
  if(len <= 0)          return -1;
  if(offset+len > size) return -1;
  ptr = user_adr + offset;
  mylock(mutex,1);
  memcpy(buf,ptr,len);
  myunlock(mutex);
  return len;
}

//==================================

void shm_init(const char *shmname, unsigned long Size){
    struct shmid_ds buf;
    FILE *fp;
    int file_existed;

    status  = OK;
    name = malloc(strlen(shmname)+1);
    memset(name, 0, strlen(shmname)+1 );
    strcpy(name,shmname);

    size    = Size + sizeof(*mutex);

    // create file
    file_existed = 1;
    fp = fopen(name,"r");
    if(fp == NULL)
    {
        file_existed = 0;
        fp = fopen(name,"w");
        if(fp == NULL)     
        {
            int ret; 
            char buf[1024];
            sprintf(buf,"could not write shm=%s\n",shmname);
            ret = shm_write(1,buf,strlen(buf));
            if(ret < 0) exit(-1);
            sprintf(buf,"you have to run this program as root !!!\n");
            ret = shm_write(1,buf,strlen(buf));
            if(ret < 0) exit(-1);
            status=ERROR_FILE;
            exit(-1);
        }
    }
    fclose(fp);

    shmkey  = ftok(name, 'b');

    id  = shmget(shmkey, size, 0666 | IPC_CREAT);
    if(id < 0)           { status=ERROR_SHMGET; return; }

    base_adr = (char *) shmat(id,NULL,0);
    if(base_adr == NULL) { status=ERROR_SHMAT;  return; }

    if(shmctl(id, IPC_STAT, &buf) != 0) { status=ERROR_SHMCTL; return; };

    mutex     = (pthread_mutex_t *) base_adr;
    user_adr  = base_adr + sizeof(*mutex);

    if(file_existed == 0) myinit(mutex);

}

//==============================
int shm_readInt(unsigned long offset, int index)
{
  int val;
  if(index < 0) return -1;
  shm_read(offset+index*sizeof(val),&val,sizeof(val));
  return val;
}

int shm_readShort(unsigned long offset, int index)
{
  short int val;
  if(index < 0) return -1;
  shm_read(offset+index*sizeof(val),&val,sizeof(val));
  return val;
}

int shm_readByte(unsigned long offset, int index)
{
  char val;
  if(index < 0) return -1;
  shm_read(offset+index*sizeof(val),&val,sizeof(val));
  return val;
}

float shm_readFloat(unsigned long offset, int index)
{
  float val;
  if(index < 0) return -1;
  shm_read(offset+index*sizeof(val),&val,sizeof(val));
  return val;
}

int shm_writeInt(unsigned long offset, int index, int val)
{
  int ret;
  if(index < 0) return -1;
  ret = shm_write(offset+index*sizeof(val),&val,sizeof(val));
  return ret;
}

int shm_writeShort(unsigned long offset, int index, int val)
{
  int ret;
  short int val2;

  if(index < 0) return -1;
  val2 = (short int) val;
  ret = shm_write(offset+index*sizeof(val2),&val2,sizeof(val2));
  return ret;
}

int shm_writeByte(unsigned long offset, int index, unsigned char val)
{
  int ret;
  if(index < 0) return -1;
  ret = shm_write(offset+index*sizeof(val),&val,sizeof(val));
  return ret;
}

int shm_writeFloat(unsigned long offset, int index, float val)
{
  int ret;
  if(index < 0) return -1;
  ret = shm_write(offset+index*sizeof(val),&val,sizeof(val));
  return ret;
}

void *shm_getUserAdr()
{
  return (void *) user_adr;
}

//==================================

void shared_memory_init( void ) {
    int size = 0;
    size = get_shmem_size();
    shm_init( "/dev/shm/wsi", size);

    printf("size: %d status: %d\n", size, status);
}

#define NAME_LEN 10
typedef struct _mem_var_type {
    char name[NAME_LEN];
    char typ;
    int size;
    unsigned int uid;
    unsigned int offset;
} mem_var;

typedef enum _memory_vars_type {
    MY_CNT = 0,
    MY_SEC
} MEMORY_VARS;

mem_var Vars[] = 
{
    {
        .name = "my_cnt"
    },
    {
        .name = "my_sec"
    }
};

void vars_init(void){
    int i,l;
    sqlite3_stmt *res;

    
    if (db_init("/projects/settings.sqlite")) {

        shared_memory_init();

        l = sizeof(Vars) / sizeof(mem_var) - 1;
        for (i = 0; i<=l; ++i) {
            Vars[i].size = -1;

            int rc = sqlite3_prepare_v2(db, "SELECT uid FROM variables_0 WHERE name = 'name' and value = ?", -1, &res, 0);
            if(rc == SQLITE_OK) {
                rc = sqlite3_bind_text(res, 1, Vars[i].name, strlen(Vars[i].name), NULL);
                int step = sqlite3_step(res);
                if (step == SQLITE_ROW) {
                    Vars[i].uid = sqlite3_column_int(res,0);

                    rc = sqlite3_prepare_v2(db, "SELECT value FROM variables_0 WHERE name = 'c_offset' and uid = ?", -1, &res, 0);
                    if(rc == SQLITE_OK){
                        rc = sqlite3_bind_int(res, 1, Vars[i].uid);
                        step = sqlite3_step(res);
                        if (step == SQLITE_ROW) {
                            Vars[i].offset = sqlite3_column_int(res,0);

                            rc = sqlite3_prepare_v2(db, "SELECT value FROM variables_0 WHERE name = 'size' and uid = ?", -1, &res, 0);
                            if(rc == SQLITE_OK){
                                rc = sqlite3_bind_int(res, 1, Vars[i].uid);
                                step = sqlite3_step(res);
                                if (step == SQLITE_ROW) {
                                    Vars[i].size = sqlite3_column_int(res,0);

                                    rc = sqlite3_prepare_v2(db, "SELECT value FROM variables_0 WHERE name = 'type' and uid = ?", -1, &res, 0);
                                    if(rc == SQLITE_OK){
                                        rc = sqlite3_bind_int(res, 1, Vars[i].uid);
                                        step = sqlite3_step(res);
                                        if (step == SQLITE_ROW) {
                                            const unsigned char * t = sqlite3_column_text(res,0);
                                            if (strncmp("int", (const char *)t, strlen((const char *)t)) == 0) Vars[i].typ ='i';
                                            if (strncmp("long", (const char *)t, strlen((const char *)t)) == 0) Vars[i].typ ='l';
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            printf("%s %d %d %d %c\n",Vars[i].name, Vars[i].uid, Vars[i].offset, Vars[i].size, Vars[i].typ);
            if (Vars[i].typ == 'i') {
              unsigned short v = -1;
              shm_read(Vars[i].offset, &v, Vars[i].size);
              printf("value: %0x %d %u\n", v, v, v);
            }
        }
        db_close();
    }
}