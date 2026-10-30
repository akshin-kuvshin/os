// author: Danila "akshin_" Axyonov

// Compiler: gcc
// Output: main.out
// Flags: -pthread

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iso646.h>
#include <pthread.h>

#include "./include/monitor.h"
#include "./include/workers.h"

int main() {
    monitor_t* monitor = malloc(sizeof(monitor_t));
    assert(monitor and "couldn't allocate memory for monitor");
    monitor_init(monitor);

    pthread_t* provider_thread = malloc(sizeof(pthread_t));
    assert(provider_thread and "couldn't allocate memory for provider thread");
    pthread_t* consumer_thread = malloc(sizeof(pthread_t));
    assert(consumer_thread and "couldn't allocate memory for consumer thread");

    int error = pthread_create(
        provider_thread,
        NULL,
        provider_worker,
        monitor);
    assert(not error and "couldn't create provider thread");
    error = pthread_create(
        consumer_thread,
        NULL,
        consumer_worker,
        monitor
    );
    assert(not error and "couldn't create consumer thread");

    pthread_join(*provider_thread, NULL);
    pthread_join(*consumer_thread, NULL);

    monitor_destroy(monitor);
    free(monitor);
    free(provider_thread);
    free(consumer_thread);

    printf("done.\n");
    return 0;
}
