#ifndef MYPTHREAD_H
#define MYPTHREAD_H

#define _GNU_SOURCE

/* in order to use the built-in Linux pthread library as a control for benchmarking, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <ucontext.h>


typedef uint mypthread_t;

/* add important states in a thread control block */
typedef struct threadControlBlock {
    ushort status; // 0 = ready, 1 = running, 2 = blocked, 3 = terminated
    mypthread_t threadID;
    uint threadPriority;
    ucontext_t *currentContext, *threadContext;
    void* returnValue;
} tcb;

/* mutex struct definition */
typedef struct mypthread_mutex_t {
    atomic_bool lock;
    mypthread_t owner;
    void *waitingQueue; // To avoid circular dependencies, we make this void * and cast it to a Queue * when we use it. This is the waiting queue for the mutex.
} mypthread_mutex_t;

// Queue data structure assigned in queue.h and queue.c. Makefile has been updated accordingly.

/* Function Declarations: */

void setupTimer(); // Setup the timer for the quantum
void create_schedule_context(); // Create the context for the scheduler


/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initialize a mutex */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif //THREADINGTEST_MYPTHREAD_H
