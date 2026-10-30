// author: Danila "akshin_" Axyonov

#ifndef MONITOR_H
#define MONITOR_H



#include <stdbool.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t* mutex;
    pthread_cond_t* can_produce;
    pthread_cond_t* can_process;
    bool ready;
} monitor_t;

void monitor_init(monitor_t* monitor);
void monitor_destroy(monitor_t* monitor);



#endif
