#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>

#include <config.h>
#include <shmem.h>
#include <vars.h>
#include <vardefs.h>

sqlite3 *db;
//====================================
int db_init(const char * name){

    int rc = sqlite3_open(name, &db);
    if (rc != SQLITE_OK) {
        syslog(LOG_INFO, "Cannot open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    return 1;
}

void db_close(void){
    sqlite3_close(db);
    syslog(LOG_INFO, "Close db");
}
//====================================

int vars_mem_size(){
//    char *err_msg = 0;
    
    sqlite3_stmt *res;
    unsigned int uid = 0;
    unsigned int max_offset = 0, var_size = 0;
    int step;
        
    int rc = sqlite3_prepare_v2(db, "SELECT max(value) FROM variables_0 WHERE name = 'c_offset'", -1, &res, 0);
    if (rc == SQLITE_OK) {
        step = sqlite3_step(res);
        if (step == SQLITE_ROW) {
            const unsigned char * tmp = sqlite3_column_text(res, 0);
            max_offset = atoi((const char *)tmp);
            sqlite3_reset(res);
        }
    }
    else
    {
        syslog(LOG_INFO, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    rc = sqlite3_prepare_v2(db,"select uid from variables_0 where name='c_offset' and value=?",-1,&res,0);
    if(rc == SQLITE_OK) {
        rc = sqlite3_bind_int(res, 1, max_offset);
        step = sqlite3_step(res);
        if (step == SQLITE_ROW) {
            uid = sqlite3_column_int(res,0);
            sqlite3_reset(res);
        }
    }
    else
    {
        syslog(LOG_INFO, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        return 0;
    }
                
    rc = sqlite3_prepare_v2(db,"select value from variables_0 where uid=? and name='size'",-1,&res,0);
    if (rc == SQLITE_OK){
        rc = sqlite3_bind_int(res, 1, uid);
        step = sqlite3_step(res);
        if (step == SQLITE_ROW) {
            var_size = sqlite3_column_int(res,0);
            sqlite3_reset(res);
        }
    }
    else
    {
        syslog(LOG_INFO, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_finalize(res);
    return max_offset + var_size;
}

void vars_init(int vars_num){
    int i;
    sqlite3_stmt *res;
    
    if (db_init("/projects/settings.sqlite")) {
        int size = 0;
        size = vars_mem_size();
        if (size == 0) {
            sqlite3_close(db);
            syslog(LOG_INFO, "could not detect shared memory size");
            exit(1);
        }

        shared_memory_init( size );

        for (i = 0; i < vars_num; ++i) {  //_MEMORYVARS_LEN_
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
                                            // TODO: other types ...
                                            if (Vars[i].size >0) {
                                                Vars[i].value = malloc(Vars[i].size);
                                                shm_read(Vars[i].offset, Vars[i].value, Vars[i].size);
                                                if (Vars[i].callback != NULL) {
                                                    Vars[i].value_old = malloc(Vars[i].size);
                                                    memcpy( Vars[i].value_old, Vars[i].value, Vars[i].size);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (opt_verbose > 0) {
                syslog(LOG_INFO, "%s %d %d (%0x) %d %c",Vars[i].name, Vars[i].uid, Vars[i].offset, Vars[i].offset, Vars[i].size, Vars[i].typ);
                if (Vars[i].typ == 'i') {
                    syslog(LOG_INFO, " %s: %0x",Vars[i].name, (short)*((short *)Vars[i].value) );
                }
            }
        }

        sqlite3_finalize(res);
        db_close();
    }
}

int vars_sleep(long msec)
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

//void vars_checker(){
void * vars_checker(void *arg){

    int vars_num = *((short *)arg);

    while (1){
        for (int i = 0; i < vars_num; ++i) {  //_MEMORYVARS_LEN_
            shm_read(Vars[i].offset, Vars[i].value, Vars[i].size);
            if(Vars[i].callback != NULL){
                if ( memcmp( Vars[i].value, Vars[i].value_old, Vars[i].size) != 0) {
                    (Vars[i].callback)(VARS_CHANGE_VALUE, &Vars[i]);
                    memcpy( Vars[i].value_old, Vars[i].value, Vars[i].size);
                }
            }
        }
        vars_sleep(10);
    }
}