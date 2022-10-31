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

#define MAX_THREADS 150 // Maximum number of threads that can be created

static void schedule();
void scheduler_interrupt_handler(int callFromFunction);
void stop_timer();

static struct sigaction sa;
static struct itimerval timer;
static int continue_scheduling = 1, threadCount = 0, schedulerInitalized = 0;
static tcb *currentThreadControlBlock = NULL;
static ucontext_t *scheduler_context, *default_context;
struct Queue *readyQueue = NULL, *blockedQueue = NULL;

// Store a thread's tcb* in the threads array, where the index is the threadId
// When calls to pthread_exit or pthread_join are made then look up the thread using this array
static tcb **threads;

void cleanUp() {
    // Free all the memory allocated for the threads
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] != NULL) {
            free(threads[i]->threadContext->uc_stack.ss_sp);
            free(threads[i]->threadContext);
            free(threads[i]);
        }
    }
    free(threads);
    freeQueue(readyQueue);
    freeQueue(blockedQueue);
    free(scheduler_context->uc_stack.ss_sp);
    free(scheduler_context);
    free(default_context->uc_stack.ss_sp);
    free(default_context);
}

int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function) (void*), void *arg) {

    printf("CREATING PTHREAD\n");

    if (threadCount == 0 && readyQueue == NULL && blockedQueue == NULL) {
        default_context = (ucontext_t *) malloc(sizeof(ucontext_t));
        checkMalloc(default_context);

        if (getcontext(default_context) == -1) {
            perror("getcontext");
            return -1;
        }

        readyQueue = initQueue();
        blockedQueue = initQueue();

        threads = (tcb**) malloc(MAX_THREADS * sizeof(tcb*));

        atexit(cleanUp);
    }

    static pid_t threadId = 0;
    ucontext_t *currentContext = malloc(sizeof(ucontext_t));
    checkMalloc(currentContext);
    ucontext_t *ucontext_thread = malloc(sizeof(ucontext_t));
    checkMalloc(ucontext_thread);

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
    ucontext_thread->uc_stack.ss_flags = 0; //
    ucontext_thread->uc_link = currentContext; // When the thread finishes, it will return to the current context
    makecontext(ucontext_thread, (void *) function, 1, arg);


    tcb *threadControlBlock = malloc(sizeof(tcb));
    threadControlBlock->currentContext = currentContext;
    threadControlBlock->threadContext = ucontext_thread;
    threadControlBlock->threadID = threadId++;
    threadControlBlock->status = 0; // 0 = ready, 1 = running, 2 = blocked, 3 = terminated

    // Need to set the given mypthread_t *thread to the threadId of the thread we just created
    *thread = (uint) threadId;

    // Save the newly created thread's tcb in the threads[], the index is the threadId
    threads[threadId] = threadControlBlock;
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
    stop_timer();
    currentThreadControlBlock->status = 0;
    scheduler_interrupt_handler(0);

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
    // preserve the return value pointer if not NULL
    // deallocate any dynamic memory allocated when starting this thread

    stop_timer();

    // Check if value_ptr is NULL
    // If is not null then save the return value to the TCB
    if(value_ptr != NULL){
        currentThreadControlBlock->returnValue = value_ptr;
    }

    // Change the status of the currently running thread to TERMINATED (3)
    currentThreadControlBlock->status = 3;


    // Check if there are any other threads waiting for this one to join
    // If there are them they need to be added back to the ready queue and allowed to run
    if(currentThreadControlBlock->numberOfThreadsWaitingToJoin > 0){
        // There are threads that are waiting for this thread to join
        // Change the status of each of those threads from waiting (2) to READY (0)
        // Add them to the ready queue

        int i = currentThreadControlBlock->numberOfThreadsWaitingToJoin;
        tcb* waitingThread;

        while(i > 1){
            // Get the threadID of the waitingThread
            --i;
            mypthread_t threadID = currentThreadControlBlock->threadsWaitingToJoin[i];
            // Get the tcb* to the waitingThread
            waitingThread = threads[threadID];
            // Set the waitingThread's status to READY
            waitingThread->status = 0;

            // Enqueue the waiting thread back to the ready queue based on scheduling algorithm 
            if(SCHED == 0){
                // RR Scheduling Algorithm
                normalEnqueue(readyQueue,waitingThread);
            }

            else{
                // PSJF Scheduling Algorithm
                priorityEnqueue(readyQueue,waitingThread);

            }
            // Do the same for the rest of the waiting threads

        }



    }

    // Deallocate any dynamic memory allocated when starting this thread

    return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

	// wait for a specific thread to terminate

    // Stop the interrupt timer when running library code
    stop_timer();

    // Get the TCB of the thread that we want to join with 
    tcb* joiningThreadTCB = threads[thread];

    // Check if the joiningThread has already terminated, if it has then continue
    if(joiningThreadTCB->status==3){
        if(value_ptr != NULL){
            *value_ptr = joiningThreadTCB->returnValue;
        }
    }

    else{
        // joiningThread has not yet terminated
        // Add the currently running thread to the threadsWaitingToJoin array of the TCB
        // Increment the numberOfThreadsWaitingToJoin to reflect that the currently running thread will now wait
        // Save the context of the currently running thread and swap to the scheduler context
        joiningThreadTCB->threadsWaitingToJoin[joiningThreadTCB->numberOfThreadsWaitingToJoin] = currentThreadControlBlock->threadID;
        joiningThreadTCB->numberOfThreadsWaitingToJoin++;
        scheduler_interrupt_handler(1);

        if(value_ptr!=NULL){
            *value_ptr = joiningThreadTCB->returnValue;
        }
    }




	// deallocate any dynamic memory created by the joining thread
    // What should be deallocated from the joining thread, that has not already been deallocated by pthread_exit()

    // What should the return values be for this function?
	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    assert(mutexattr == NULL); // We don't support mutex attributes
	
	mutex = malloc(sizeof(mypthread_mutex_t));
    checkMalloc(mutex); // Will exit with return status -1 if failed.

    atomic_flag_clear(&mutex->lock);

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
        if (atomic_flag_test_and_set(&mutex->lock)) {
            // Lock is already set, so we need to block the thread
            currentThreadControlBlock->status = 2;
            currentThreadControlBlock->threadPriority = 0; // We set the priority to 0 so that it is the first to be scheduled when it is unblocked.

            if (SCHED) {
                priorityEnqueue(blockedQueue, currentThreadControlBlock); // FIXME: Will probably cause a memory issue.
            } else {
                normalEnqueue(blockedQueue, currentThreadControlBlock);
            }

            return -1;
        } else {
            return 0;
        }
	
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
	// YOUR CODE HERE	
    atomic_flag_clear(&mutex->lock);
    currentThreadControlBlock->status = 0;
    currentThreadControlBlock->threadPriority = 0;

    priorityEnqueue(readyQueue, currentThreadControlBlock);

	return 0; // What is the return value here.
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
    free(mutex);
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
void scheduler_interrupt_handler(int callFromFunction){
    // Before entering library code the scheduler interrupt timer must be stopped
    stop_timer();


    printf("Interrupt Recived\n");
    if (currentThreadControlBlock == NULL) {
        // No thread was executing, just swap to the schedule() context
       //getcontext(default_context);
        //setcontext(scheduler_context);
        setcontext(scheduler_context);
        //printf("Switching to scheduler context\n");
       
    }

    else if (callFromFunction == 0){
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

    else if(callFromFunction == 1){
        // There is currently a thread executing but it has made a call to mypthread_join()
        // The thread it wants to join with has not finished terminating, therefore we must stop executing the current thread until the joing thread has finished
        // Swap context from the currently running thread to the scheduler context so another thread can run
        // The currently running thread will not be added to the ready queue, when the joining thread calls mypthread_exit() it will be handled by that function
        swapcontext(currentThreadControlBlock->threadContext,scheduler_context);

    }

    else if(callFromFunction == 2){
        // Pthread_exit() has just been called, the currently running pthread is the one that called it
        // Since thread has finished runnning, no need to save it's context, just swap to scheduler's context
        setcontext(scheduler_context);
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

    //printf("Scheduler context has been setup\n");
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
    
    while (continue_scheduling == 1) {

        printf("Running scheduler\n");
        if(!isEmpty(readyQueue) && SCHED == 0) {
            // Call the Round Robin scheduling algorithm
            // If successfull, sets the currentThreadControlBlock variable to that of the next thread to run
            printf("Running RR algo\n");
            sched_RR();
            restart_timer();
            swapcontext(scheduler_context,currentThreadControlBlock->threadContext);
            
        }

        else if(!isEmpty(readyQueue) && SCHED == 1){
            // Call the PSJF scheduling algorithm
            // If successfull, sets the currentThreadControlblock variable to that of the next thread to run.
            printf("Running PSJF algo\n");
            sched_PSJF();
            restart_timer();
            swapcontext(scheduler_context,currentThreadControlBlock->threadContext);
        }

        else {
            swapcontext(scheduler_context,default_context);
        }
    }

	return;
}

// Feel free to add any other functions you need

// YOUR CODE HERE
