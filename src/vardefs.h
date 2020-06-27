#ifndef _VARDEFS_H_
#define _VARDEFS_H_

#define NAME_LEN 10

typedef struct _mem_var_type {
    char name[NAME_LEN];
    char typ;
    int size;
    unsigned int uid;
    unsigned int offset;
    void *value;
} mem_var;

typedef enum _memory_vars_type {
    MY_CNT = 0,
    MY_SEC,
    _MEMORYVARS_LEN_
} MEMORY_VARS;

extern mem_var Vars[];

#endif
