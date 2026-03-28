/**
 * @file uvector_pthread_conf.h
 * @brief dalloc configuration for uvector thread safety tests with POSIX threads
 */

#ifndef UVECTOR_PTHREAD_CONF_H
#define UVECTOR_PTHREAD_CONF_H

#include <pthread.h>
#include <stdlib.h>

/* Enable thread safety with recursive mutex */
#define USE_THREAD_SAFETY

/* POSIX threads recursive mutex implementation */
#define DALLOC_MUTEX_TYPE           pthread_mutex_t*

#define DALLOC_MUTEX_CREATE()       ({ \
    pthread_mutex_t *m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)); \
    if (m) { \
        pthread_mutexattr_t attr; \
        pthread_mutexattr_init(&attr); \
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init(m, &attr); \
        pthread_mutexattr_destroy(&attr); \
    } \
    m; \
})

#define DALLOC_MUTEX_DELETE(mutex)  do { \
    if (mutex) { pthread_mutex_destroy(mutex); free(mutex); } \
} while(0)

#define DALLOC_MUTEX_LOCK(mutex)    do { if (mutex) pthread_mutex_lock(mutex); } while(0)
#define DALLOC_MUTEX_UNLOCK(mutex)  do { if (mutex) pthread_mutex_unlock(mutex); } while(0)

/* 8-byte alignment for structs with pointer members */
#define ALLOCATION_ALIGNMENT_BYTES  8U

/* Disable debug output for cleaner test output */
#define dalloc_debug(...)
#define uvector_debug(...)

#endif /* UVECTOR_PTHREAD_CONF_H */
