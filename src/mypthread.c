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
void scheduler_interrupt_handler();
void stop_timer();
void restart_timer();

static struct sigaction sa;
static struct itimerval timer;
static int continue_scheduling = 1, threadCount = 0, schedulerInitalized = 0,timerInitalized = 0;
static int interruptCaller = 0; // 0 -> periodic timer interrupt or mypthread yield(), 1 -> mypthread join(), 2 -> mypthread exit()
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

    if(timerInitalized == 1){
        stop_timer();
    }

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
    threadControlBlock->threadID = threadCount;
    threadControlBlock->status = 0; // 0 = ready, 1 = running, 2 = blocked, 3 = terminated
    threadControlBlock->numberOfThreadsWaitingToJoin = 0;
    threadControlBlock->threadsWaitingToJoin = (mypthread_t**) malloc(MAX_THREADS * sizeof(mypthread_t*));
    // Need to set the given mypthread_t *thread to the threadId of the thread we just created
    // Save the newly created thread's tcb in the threads[], the index is the threadId
    threads[threadControlBlock->threadID] = (tcb*) malloc(sizeof(tcb));
    threads[threadControlBlock->threadID] = threadControlBlock;

    *thread = threadControlBlock->threadID;
    threadCount++;

    

    if (SCHED) {
        threadControlBlock->threadPriority = -1; // We are not concerned about priority in this case.
        normalEnqueue(readyQueue, threadControlBlock);
    } else {
        threadControlBlock->threadPriority = 0;
        priorityEnqueue(readyQueue, threadControlBlock);
    }

    if (schedulerInitalized == 0) {
        create_schedule_context();
        setup_timer();
        schedulerInitalized = 1;
        timerInitalized = 1;
    }

    if(timerInitalized == 1){
        restart_timer();
    }

    return 0;
}

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield() {
    stop_timer();
    currentThreadControlBlock->status = 0;
    scheduler_interrupt_handler();

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {

    stop_timer();

    // if value_ptr is not NULL then save the value to be returned in the tcb
    if(value_ptr != NULL){
        currentThreadControlBlock->returnValue = value_ptr;
    }

    
    // Change the status of the currently running thread to TERMINATED (3)
    currentThreadControlBlock->status = 3;


    // Check if there are any other threads waiting for this one to join
    if(currentThreadControlBlock->numberOfThreadsWaitingToJoin > 0){
        // There are threads that are waiting for this thread to join
  
        int numOfThreads = currentThreadControlBlock->numberOfThreadsWaitingToJoin;
        tcb* waitingThread;

        // Get each thread that was waiting for the current thread and add them to the ready queue
        while(numOfThreads > 1){
            
            --numOfThreads;

            // Get the threadID of a waiting thread
            mypthread_t threadID = *currentThreadControlBlock->threadsWaitingToJoin[numOfThreads];

            // Use the threadID to look up the waiting thread and get its tcb
            waitingThread = threads[threadID];

            // Set the waitingThread's status to READY (0)
            waitingThread->status = 0;

            // Enqueue the waiting thread back to the ready queue based on scheduling algorithm 
            if(SCHED){
                // RR Scheduling Algorithm
                normalEnqueue(readyQueue,waitingThread);
            }

            else{
                // PSJF Scheduling Algorithm
                priorityEnqueue(readyQueue,waitingThread);

            }

        }



    }


    // Deallocate any dynamic memory allocated when starting this thread

    // Deallocate the currentContext, threadContext and threadsWaitingToJoin because they will not longer be used
    // Do not deallocate the actual tcb, the information contained in it may still be needed by mypthread_join()
    // free(currentThreadControlBlock->currentContext->uc_stack.ss_sp);
    // free(currentThreadControlBlock->currentContext);
    // free(currentThreadControlBlock->threadContext->uc_stack.ss_sp);
    // free(currentThreadControlBlock->threadContext);
    //free(currentThreadControlBlock->threadsWaitingToJoin);

    // Switch to scheduler context
    interruptCaller = 2;
    scheduler_interrupt_handler();

    return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {

    // Stop the interrupt timer when running library code
    stop_timer();

    // Get the tcb of the thread that the calling thread is waiting for
    tcb* exitThreadTCB = threads[thread];


   printf("%d\n",threadCount);

    // Check if the exitThread has already exited
    if(exitThreadTCB->status==3){
        // The exitThread has already exited

        // If value_ptr is not NULL, import the return value
        if(value_ptr != NULL){
            *value_ptr = exitThreadTCB->returnValue;
        }
    }

    else{
        // The exitThread has not yet exited

        // Increment the numberOfThreadsWaitingToJoin, Add the calling thread to the exitThread's tcb
        exitThreadTCB->numberOfThreadsWaitingToJoin++;
        exitThreadTCB->threadsWaitingToJoin[exitThreadTCB->numberOfThreadsWaitingToJoin] = malloc(sizeof(mypthread_t));
        *exitThreadTCB->threadsWaitingToJoin[exitThreadTCB->numberOfThreadsWaitingToJoin] = currentThreadControlBlock->threadID;


        
        interruptCaller = 1;
        scheduler_interrupt_handler();


         // If value_ptr is not NULL, import the return value
        if(value_ptr!=NULL){
            *value_ptr = exitThreadTCB->returnValue;
        }
    }




	// deallocate any dynamic memory created by the joining thread

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
                normalEnqueue(blockedQueue, currentThreadControlBlock);
            } else {
                priorityEnqueue(blockedQueue, currentThreadControlBlock); // FIXME: Will probably cause a memory issue.
            }

            scheduler_interrupt_handler(2);

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
    tcb *threadToEnqueue = (tcb *) normalDequeue(blockedQueue); // As per directions

    if (!SCHED) {
        threadToEnqueue->threadPriority++; // Decrease the priority of the thread that just released the lock
    }

    if (SCHED) {
        normalEnqueue(readyQueue, threadToEnqueue);
    } else {
        priorityEnqueue(readyQueue, threadToEnqueue);
    }

	return 0; // What is the return value here?
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
    free(mutex);
    return 0;
};




/* Timer functions */

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





/* scheduler */

/* The scheduler's context must be created before it can be switched into periodically*/
void create_schedule_context() {
    scheduler_context = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(scheduler_context);

    scheduler_context->uc_stack.ss_sp = malloc(STACKSIZE);
    scheduler_context->uc_stack.ss_size = STACKSIZE;
    scheduler_context->uc_stack.ss_flags = 0;
    scheduler_context->uc_link = default_context;


    makecontext(scheduler_context,schedule,0,NULL);
}


/* Handle the timer interrupt and run scheduler*/
void scheduler_interrupt_handler(){
    // Before entering library code the scheduler interrupt timer must be stopped
    stop_timer();


    if (currentThreadControlBlock == NULL) {
        // No thread was executing, just swap to the schedule() context
        setcontext(scheduler_context);
    }

    else if (interruptCaller == 0){ // Either the periodic timer interrupt happend or mypthread yield() was just called
        // The currently running thread will be enqueued at the back of the ready queue.
        // If the scheduling algorithm is PSJF then the threadPriority will be increased
        
        if (SCHED) {
            normalEnqueue(readyQueue,currentThreadControlBlock);
        }

        else{
            currentThreadControlBlock->threadPriority++;
            priorityEnqueue(readyQueue,currentThreadControlBlock);
        }

       // Save the currently running thread's context, and swap to the scheduler's context
        swapcontext(currentThreadControlBlock->threadContext,scheduler_context);
    }

    else if(interruptCaller == 1){ // mypthread join() was just called  
        // Reset interruptCaller back to 0
        interruptCaller  = 0;

        // The currently running thread is waiting on another thread to finish, do not add to the ready queue
        // Save the currently running thread's context, and swap to the scheduler's context

        swapcontext(currentThreadControlBlock->threadContext,scheduler_context);

    }

    else if (interruptCaller == 2){ // mypthread exit() was just called
        // Reset interruptCaller back to 0
        interruptCaller  = 0;

        // The currently running thread has terminated, it's context does not need to be saved
        // Switch to the scheduler context to run next thread
        setcontext(scheduler_context);
    }

}


/* Round Robin scheduling algorithm */
static void sched_RR()
{
    // Dequeue thread from ready queue
    currentThreadControlBlock = normalDequeue(readyQueue);
	return;
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
    // Dequeue the thread with the highest priority
    currentThreadControlBlock = normalDequeue(readyQueue);
	return;
}


static void schedule() {


    while (continue_scheduling == 1) {

        if(!isEmpty(readyQueue) && SCHED == 0) {
            // Call the RR Scheduling algorithm, it will make currentThreadControlBlock point to the next thread to run
            sched_RR();

            // Restart the timer so that the scheduler can periodically run
            restart_timer();

            // Switch from the scheduler's context to the context of the thread that the RR algorithm chose
            swapcontext(scheduler_context,currentThreadControlBlock->threadContext);
            
        }

        else if(!isEmpty(readyQueue) && SCHED == 1){
            // Call the PSJF scheduling algorithm, it will make currentThreadControlBlock point to the next highest priority thread to run

            sched_PSJF();

            // Restart the timer so that the scheduler can periodically run
            restart_timer();

            // Switch from the scheduler's context to the context of the thread that the PSJF algorithm chose
            swapcontext(scheduler_context,currentThreadControlBlock->threadContext);
        }

        else {
            // Restart the timer so that the scheduler can periodically run
            restart_timer();

            // The readyQueue is empty therefore there are no threads to run
            // Switch from the scheduler's context to the default context
            swapcontext(scheduler_context,default_context);
        }
    }

	return;
}

// Feel free to add any other functions you need

// YOUR CODE HERE
