// author: Danila "akshin_" Axyonov

#include <stdlib.h>
#include <assert.h>
#include <iso646.h>
#include <pthread.h>

#include "./include/monitor.h"

void monitor_init(monitor_t* monitor) {
    monitor->mutex = malloc(sizeof(pthread_mutex_t));
    assert(monitor->mutex and "couldn't allocate memory for mutex");
    int error = pthread_mutex_init(monitor->mutex, NULL);
    assert(not error and "couldn't initialize mutex");

    monitor->can_produce = malloc(sizeof(pthread_cond_t));
    assert(monitor->can_produce and "couldn't allocate memory for \"can_produce\" cond");
    error = pthread_cond_init(monitor->can_produce, NULL);
    assert(not error and "couldn't initialize \"can_produce\" cond");

    monitor->can_process = malloc(sizeof(pthread_cond_t));
    assert(monitor->can_process and "couldn't allocate memory for \"can_process\" cond");
    error = pthread_cond_init(monitor->can_process, NULL);
    assert(not error and "couldn't initialize \"can_process\" cond");

    monitor->ready = false;

    monitor->data = 0;
}

void monitor_destroy(monitor_t* monitor) {
    int error = pthread_mutex_destroy(monitor->mutex);
    assert(not error and "couldn't destroy mutex");

    error = pthread_cond_destroy(monitor->can_produce);
    assert(not error and "couldn't destroy \"can_produce\" cond");

    error = pthread_cond_destroy(monitor->can_process);
    assert(not error and "couldn't destroy \"can_process\" cond");
}
