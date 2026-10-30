// author: Danila "akshin_" Axyonov

#include <stdio.h>
#include <time.h>
#include <iso646.h>
#include <pthread.h>

#include "./include/monitor.h"

const struct timespec second = { .tv_sec = 1, .tv_nsec = 0 };

void* provider_worker(void* arg) {
    monitor_t* monitor = (monitor_t*)arg;

    int cnt = 0;
    while (true) {
        pthread_mutex_lock(monitor->mutex);

        while (monitor->ready)
            pthread_cond_wait(monitor->can_produce, monitor->mutex);

        monitor->ready = true;
        printf("provider produced event #%d\n", ++cnt);
        pthread_cond_signal(monitor->can_process);

        pthread_mutex_unlock(monitor->mutex);
        nanosleep(&second, NULL);
    }

    return NULL;
}

void* consumer_worker(void* arg) {
    monitor_t* monitor = (monitor_t*)arg;

    int cnt = 0;
    while (true) { // этот цикл не для ожидания события, т.е. не вместо pthread_cond_wait!
        pthread_mutex_lock(monitor->mutex);

        while (not monitor->ready)
            pthread_cond_wait(monitor->can_process, monitor->mutex);

        // в каком-то смысле, здесь поток вывода stdout также становится критическим ресурсом
        printf("consumer processed event #%d\n", ++cnt);
        monitor->ready = false;
        pthread_cond_signal(monitor->can_produce);

        pthread_mutex_unlock(monitor->mutex);
    }

    return NULL;
}
