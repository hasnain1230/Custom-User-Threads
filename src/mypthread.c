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

//static ucontext_t* scheduler_context;
static int continue_scheduling = 1;
static tcb* currentThreadControlBlock = NULL;
//static ucontext_t* default_context;

static ucontext_t *scheduler_context, *default_context;

static int threadCount = 0;
struct Queue *readyQueue = NULL;
static struct Queue *blockedQueue = NULL;


// // Required to periodically switch to the scheduler context
// // Need to save the scheduler context globally to run it
// ucontext_t schedulerContext;

int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function) (void*), void *arg) {

    printf("CREATING PTHREAD\n");

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


    if (schedulerInitalized == 0) {
        create_schedule_context();
        setup_timer();
        schedulerInitalized = 1;
    }

    return 0;
}

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield() {
	// change current thread's state from Running to Ready
	threadContext->status = 0;
	// save context of this thread to its thread control block
	threadControlBlock->threadContext = threadContext;
	// switch from this thread's context to the scheduler's context
	schedule();

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// preserve the return value pointer if not NULL
	if (value_ptr != NULL){
		threadContext->value = value_ptr;
	}
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
/* After finishing running library code, restart timer to allow scheduler to run periodically*/
void restart_timer() {
    timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = QUANTUM;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = QUANTUM;
	setitimer(ITIMER_REAL,&timer,NULL);
}

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
    //      -> this function saves the currently running context into threadControlBlock->threadContext, and switches to the scheduler's context

    printf("Interrupt Recived\n");
    if(currentThreadControlBlock == NULL) {
        // No thread was executing, just swap to the schedule() context
       //getcontext(default_context);
        //setcontext(scheduler_context);
        setcontext(scheduler_context);
        printf("Switching to scheduler context\n");
       
    }

    else{
        // There is currently a thread executing
        // Increase the threads priority, and enqueue to the back of the queue
        //      if the scheduling algorithm is PSJF, then use priority enqueue, otherwise use normal enqueue
        if (SCHED == 1) {
            currentThreadControlBlock->threadPriority++;
            priorityEnqueue(readyQueue,currentThreadControlBlock);
        }

        else{
            normalEnqueue(readyQueue,currentThreadControlBlock);
        }

        // Now swap context from the currently running thread to the scheduler context
        swapcontext(currentThreadControlBlock->threadContext,scheduler_context);
    }

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


/* The scheduler's context must be created before it can be sweitched into periodically*/
void create_schedule_context() {
    scheduler_context = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(scheduler_context);

    scheduler_context->uc_stack.ss_sp = malloc(STACKSIZE);
    scheduler_context->uc_stack.ss_size = STACKSIZE;
    scheduler_context->uc_stack.ss_flags = 0;

    // What should uc_link actually point to?
    scheduler_context->uc_link = default_context;


    makecontext(scheduler_context,schedule,0,NULL);

    printf("Scheduler context has been setup\n");


}


/* scheduler */


/* Round Robin scheduling algorithm */
static void sched_RR()
{
    // The scheduler_interrupt_handler() has already stopped the clock, no need to stop it. Safe to continue working.


    // Handling RR scheduling:
    // Dequeue the next thread control block from the queue
    // Save the thread to be run's tcb to a global pointer to keep track.
    // The schedule() will take the thread that was just saved at the global pointer and it will be ran.

    currentThreadControlBlock = normalDequeue(readyQueue);



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


    currentThreadControlBlock = normalDequeue(readyQueue);

    
    // Finished running library code, context switch to the thread to be run and restart scheduler interrupt timer
    //restart_timer();

	return;
}


static void schedule() {
    // Do we need to call stop_timer() because the scheduler_interrupt_handler() already stops the timer
    //stop_timer();




    // How long should the scheduler run, should it loop until there are no more threads in the queue? Maybe like this:
    // while(queue->currentSize > 0) {
    //      Call the correct scheduling algorithm, the algorithm will update the global thread tcb pointer
    //      Swap scheduler context with the context pointed to by the global thread tcb pointer
    //      Restart timer so that the scheduler can run again in the future
    //{
    
    while(continue_scheduling == 1){

        printf("Running scheduler\n");
        if(readyQueue->currentSize > 0  && SCHED == 0) {
            // Call the Round Robin scheduling algorithm
            // If successfull, sets the currentThreadControlBlock variable to that of the next thread to run
            printf("Running RR algo\n");
            sched_RR();
            restart_timer();
            swapcontext(scheduler_context,currentThreadControlBlock->threadContext);
            
        }

        else if(readyQueue->currentSize > 0 && SCHED == 1){
            // Call the PSJF scheduling algorithm
            // If successfull, sets the currentThreadControlblock variable to that of the next thread to run.
            printf("Running PSJF algo\n");
            sched_PSJF();
            restart_timer();
            swapcontext(scheduler_context,currentThreadControlBlock->threadContext);
        }

        else{
            // There is no thread in the ready queue
            // Switch to the default context
            
            swapcontext(scheduler_context,default_context);
        }



    }
    

	return;
}

// Feel free to add any other functions you need

// YOUR CODE HERE
