// File:	mypthread.c
// List all group members' names: Della Maret, Hasnain Ali, ADD NAME
// Hasnain Ali...
// iLab machine tested on: ilab1.cs.rutgers.edu

#include "mypthread.h"
#include <stdatomic.h>
#include <ucontext.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include "queue.h"

#ifndef PSJF
#define SCHED 0 // Indicates round-robin scheduling
#else
#define SCHED 1 // Indicates PSJF scheduling
#endif

#define STACKSIZE (2 * 1024 * 1024) // According to man pthread_attr_init, the default stack size is 2MB. Keeping with this, we'll initialize the stack size to 2MiB as well, converted to bytes.
#define QUANTUM 25000 // Quantum is set to 25ms = 25000 us

static struct sigaction sa;
static struct itimerval timer;
static void schedule();
static int schedulerInitalized = 0;
static ucontext_t *scheduler_context, *default_context;

#define STACKSIZE (2 * 1024 * 1024) // According to man pthread_attr_init, the default stack size is 2MB. Keeping with this, we'll initialize the stack size to 2MiB as well, converted to bytes.

// Keep track of the number of threads so that the threadIDs are assigned properly
static int threadCount = 0;
struct Queue *readyQueue = NULL;
static struct Queue *blockedQueue = NULL;


// Required to periodically switch to the scheduler context
// Need to save the scheduler context globally to run it
ucontext_t schedulerContext;

void checkMalloc(void *ptr) {
    if (ptr == NULL) {
        perror("Malloc failed.");
        exit(1);
    }
}

int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function) (void*), void *arg) {

    if (threadCount == 0 && readyQueue == NULL && blockedQueue == NULL) {
        default_context = (ucontext_t *) malloc(sizeof(ucontext_t));
        if (getcontext(default_context) == -1) {
            perror("getcontext");
            return -1;
        }

        readyQueue = initQueue();
        blockedQueue = initQueue();
    }

    static pid_t threadId = 0;
    ucontext_t *currentContext = malloc(sizeof(ucontext_t));
    ucontext_t *ucontext_thread = malloc(sizeof(ucontext_t));

    if (getcontext(currentContext) == -1 || getcontext(ucontext_thread) == -1) { // If something when creating thread goes wrong, we clean up and return -1
        perror("getcontext failed.");
        free(currentContext);
        free(ucontext_thread);
        freeQueue(readyQueue);
        freeQueue(blockedQueue);
        return -1;
    }

    ucontext_thread->uc_stack.ss_size = STACKSIZE;
    ucontext_thread->uc_stack.ss_sp = malloc(ucontext_thread->uc_stack.ss_size);
    ucontext_thread->uc_stack.ss_flags = 0;
    ucontext_thread->uc_link = currentContext; // Might be wrong??
    makecontext(ucontext_thread, (void *) function, 1, arg); // Might need a wrapper function?

    tcb *threadControlBlock = malloc(sizeof(tcb));
    threadControlBlock->currentContext = currentContext;
    threadControlBlock->threadContext = ucontext_thread;
    threadControlBlock->threadID = threadId++;
    threadControlBlock->status = 0; // 0 = ready, 1 = running, 2 = blocked, 3 = terminated
    threadCount++;

    if (SCHED) {
        threadControlBlock->threadPriority = 0;
        priorityEnqueue(readyQueue, threadControlBlock);
    } else {
        threadControlBlock->threadPriority = -1; // We are not concerned about priority in this case.
        normalEnqueue(readyQueue, threadControlBlock);
    }

    return 0;
}

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield() {
	// change current thread's state from Running to Ready
	// save context of this thread to its thread control block
	// switch from this thread's context to the scheduler's context
	
	schedule();

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// preserve the return value pointer if not NULL
	// deallocate any dynamic memory allocated when starting this thread
	
	return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {
	// YOUR CODE HERE

	// wait for a specific thread to terminate
	// deallocate any dynamic memory created by the joining thread

	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    assert(mutexattr == NULL);
	// YOUR CODE HERE
	
	mutex = malloc(sizeof(mypthread_mutex_t));
    atomic_flag_clear(&mutex->lock);

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
		// YOUR CODE HERE
	
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
		
		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// YOUR CODE HERE	
	
	// update the mutex's metadata to indicate it is unlocked
	// put the thread at the front of this mutex's blocked/waiting queue in to the run queue

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
	// YOUR CODE HERE
	
	// deallocate dynamic memory allocated during mypthread_mutex_init

	return 0;
};




/* Scheduler helper method */


/* setup new interval timer*/

/* Before running any library code, stop the timer so that library code does not get interrupt*/
void stop_timer() {
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL,&timer,NULL);
}

/* Handle the timer interrupt and run scheduler*/
void scheduler_interrupt_handler(){

    // Before entering library code the scheduler interrupt timer must be stopped
    stop_timer();

    // Need to do the following:
    // Get current tcb pointer
    // Increase the priority of the threadControlBlock;
    // Send the current threadControlBlock to the back of the queue
    // Call swapcontext(threadControlBlock->threadContext,scheduler_context)
    //      -> saves the currently running context into threadControlBlock, and switches to the scheduler's context



}

void setup_timer() {
    // Set up the interrupt signal
	memset(&sa,0,sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
    // At every quantum interval the signal will be handled by switching to the schedule context
	sa.sa_handler = scheduler_interrupt_handler;
	sigaction(SIGALRM,&sa,NULL);
    sa.sa_flags = SA_RESTART;

    // The timer has been setup to run for the quantum time period.
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = QUANTUM;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = QUANTUM;
	setitimer(ITIMER_REAL,&timer,NULL);
}

/* After finishing running library code, restart timer to allow scheduler to run periodically*/
void restart_timer() {
    timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = QUANTUM;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = QUANTUM;
	setitimer(ITIMER_REAL,&timer,NULL);
}

/* The scheduler's context must be created before it can be sweitched into periodically*/
void create_schedule_context() {
    scheduler_context = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(scheduler_context);

    scheduler_context->uc_stack.ss_sp = malloc(STACKSIZE);
    scheduler_context->uc_stack.ss_size = STACKSIZE;
    scheduler_context->uc_stack.ss_flags = 0;

    // What should uc_link actually point to?
    scheduler_context->uc_link = 0;


    makecontext(scheduler_context,schedule,0);

    printf("Scheduler context has been setup\n");


}


/* scheduler */
static void schedule() {
    // Do we need to call stop_timer() because the scheduler_interrupt_handler() already stops the timer
    //stop_timer();




    // How long should the scheduler run, should it loop until there are no more threads in the queue? Maybe like this:
    // while(queue->currentSize > 0) {
    //      Call the correct scheduling algorithm, the algorithm will update the global thread tcb pointer
    //      Swap scheduler context with the context pointed to by the global thread tcb pointer
    //      Restart timer so that the scheduler can run again in the future
    //{


    // each time a timer signal occurs your library should switch in to this context
	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF 

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
    // The scheduler_interrupt_handler() has already stopp the clock, no need to stop it. Safe to continue working.


    // Handling RR scheduling:
    // Dequeue the next thread control block from the queue
    // Check if thread is ready, if it is ready then it will be the next to run
    //          Change status to running
    // Save the thread to be run's tcb to a global pointer to keep track.
    // The schedule() will take the thread that was just saved at the global pointer and it will be ran.



    // Finished running library code, context switch to the thread to be run and restart scheduler interrupt timer
    //restart_timer();
	
	return;
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	// YOUR CODE HERE
    
	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)
    
    // Finished running library code, context switch to the thread to be run and restart scheduler interrupt timer
    restart_timer();

	return;
}


// Feel free to add any other functions you need

// YOUR CODE HERE