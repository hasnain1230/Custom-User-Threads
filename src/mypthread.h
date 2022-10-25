// File:	mypthread_t.h

// List all group members' names:
// iLab machine tested on:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* in order to use the built-in Linux pthread library as a control for benchmarking, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>



typedef uint mypthread_t;

	/* add important states in a thread control block */
typedef struct threadControlBlock
{
	// YOUR CODE HERE	
	
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	int threadID;
	int status;
	int priority;
	ucontext_t* context;
	void* stack;


} tcb;

/* mutex struct definition */
typedef struct mypthread_mutex_t
{
	// If lock == 0, then no thread has the lock on the mutex
	// If look == 1,then there is a lock on the mutex
	int lock;

	// Should we include a queue for the mutex within the struct?
	
} mypthread_mutex_t;


// Feel free to add your own auxiliary data structures (linked list or queue etc...)


// We need a queue data strcture to hold threads that are waiting for mutex
typedef struct threadQueNode {
	tcb* currentTCB;
	threadQueNode* nextThreadNode;
} threadQueNode;

typedef struct mutexQueNode {
	tcb* currentTCB;
	threadQueuNode* nextThreadNode;
} mutexQueNode;


/* Function Declarations: */

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

#endif
