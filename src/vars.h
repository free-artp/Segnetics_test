#ifndef _VARS_H_
#define _VARS_H_

typedef enum _vars_events_type {
    VARS_CHANGE_VALUE = 1,
    VARS_NOTHING
} VARS_EVENTS;


int vars_mem_size(void);
void vars_init(int vars_num);
void * vars_checker(void *arg);

#endif