#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

#include <shmem.h>
#include <vardefs.h>

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

int vars_mem_size(){
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

void vars_init(void){
    int i;
    sqlite3_stmt *res;

    
    if (db_init("/projects/settings.sqlite")) {
        int size = 0;
        size = vars_mem_size();

        shared_memory_init( size );

        //l = sizeof(Vars) / sizeof(mem_var) - 1;
        for (i = 0; i<_MEMORYVARS_LEN_; ++i) {
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
                                            }
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
              printf("value: %0x\n", (short)*((short *)Vars[i].value) );
            }
        }

        sqlite3_finalize(res);
        db_close();
    }
}