// File:	mypthread.c

// List all group members' names: Della Maret, Hasnain Ali, ADD NAME
// Hasnain Ali...
// iLab machine tested on: ilab1.cs.rutgers.edu

#include "mypthread.h"
#include <stdatomic.h>
#include <ucontext.h>
#include <string.h>
#include <assert.h>

#define STACKSIZE (2 * 1024 * 1024) // According to man pthread_attr_init, the default stack size is 2MB. Keeping with this, we'll initialize the stack size to 2MiB as well, converted to bytes.

#ifndef PSJF
#define RR 1
#else
#define RR 0
#endif

#define STACKSIZE 30000


// Keep track of the number of threads so that the threadIDs are assigned properly
int threadCount = 0;


// Required to periodically switch to the scheduler context
// Need to save the scheduler context globally to run it
ucontext_t schedulerContext;


/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg)
{
	// create a Thread Control Block
	// create and initialize the context of this thread
	// allocate heap space for this thread's stack
	// after everything is all set, push this thread into the ready queue

	tcb* newTCB = (tcb*) malloc(sizeof(tcb*));
	newTCB->threadID = threadCount + 1;
	newTCB->context = (ucontext_t*) malloc(sizeof(ucontext_t*));
	newTCB->stack = malloc(STACKSIZE);
	newTCB->context->uc_stack.ss_sp = stack;
	newTCB->context->uc_stack.ss_size = STACKSIZE;
	newTCB->context->uc_stack.ss_flags = 0;
	newTCB->priority = 1;
	newTCB->status = 1;

	getcontext(newTCB->context);

	// HOW DO WE GET NUMBER OF ARGUMENTS PASS TO THREAD?
	// THIS NEEDS TO BE PLACED WHERE THE 1 IS CURRENTLY BELOW
	makecontext(newTCB->context,function,1);

	// Increment the number of threads
	threadCount++;

	// The tcb has been set up, now push it into the ready queue
  
	return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
	// change current thread's state from Running to Ready
	// save context of this thread to its thread control block
	// switch from this thread's context to the scheduler's context
	
	schedule();

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// preserve the return value pointer if not NULL
	// deallocate any dynamic memory allocated when starting this thread
	
	return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	// YOUR CODE HERE

	// wait for a specific thread to terminate
	// deallocate any dynamic memory created by the joining thread

	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
    assert(mutexattr == NULL);
	// YOUR CODE HERE
	
	//initialize data structures for this mutex

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
		// YOUR CODE HERE
	
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
		
		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE	
	
	// update the mutex's metadata to indicate it is unlocked
	// put the thread at the front of this mutex's blocked/waiting queue in to the run queue

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE
	
	// deallocate dynamic memory allocated during mypthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule()
{
	// each time a timer signal occurs your library should switch in to this context
	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF or MLFQ

	/* if (schedule == RR){
		sched_RR();
	}

	if (schedule == PSJF){
		sched_PSJF();
	}

	if (schedule == MLFQ){
		sched_MLFQ();
	} */

	#ifndef RR
		#ifndef PSJF
			sched_MLFQ();
		#else
			sched_RR();
		#endif
		sched_PSJF();
	#endif

	return;
}

/* Round Robin scheduling algorithm */
static void sched_RR()
{
	// YOUR CODE HERE
	
	// Your own implementation of RR
	// (feel free to modify arguments and return types)
	
	return;
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	// YOUR CODE HERE

	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)

	return;
}


// Feel free to add any other functions you need

// YOUR CODE HERE
