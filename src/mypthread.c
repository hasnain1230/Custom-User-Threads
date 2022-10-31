// File:	mypthread.c

// List all group members' names:
// iLab machine tested on:

#include "mypthread.h"
#include "queue.h"
#include "checkMalloc.h"
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>

#ifndef PSJF
#define SCHED 0 // Indicates round-robin scheduling
#else
#define SCHED 1 // Indicates PSJF scheduling
#endif

#define QUANTUM 25000 // Quantum is set to 25ms = 25000 us


// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

static struct sigaction sa;
static struct itimerval timer;

static uint threadCount = 0;
static tcb *currentThreadControlBlock = NULL;
static ucontext_t *scheduler_context, *default_context;
struct Queue *readyQueue = NULL, *blockedQueue = NULL;

void setupTimer();
void scheduler_interrupt_handler(int signalNumber);

/* create a new thread */
int mypthread_create(mypthread_t *thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
    // Creating Thread Control Block:
    if (threadCount == 0 && readyQueue == NULL && blockedQueue == NULL) {
        default_context = (ucontext_t *) malloc(sizeof(ucontext_t));
        checkMalloc(default_context);

        if (getcontext(default_context) == -1) {
            perror("getcontext");
            return -1;
        }

        readyQueue = initQueue();
        blockedQueue = initQueue();

        create_schedule_context();

        // atexit(cleanUp);
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

    ucontext_thread->uc_stack.ss_size = SIGSTKSZ;
    ucontext_thread->uc_stack.ss_sp = malloc(SIGSTKSZ);
    ucontext_thread->uc_link = currentContext; //FIXME: Is this okay? Should we go to default instead? // When the thread finishes, it will return to the current context
    makecontext(ucontext_thread, (void *) function, 1, arg);

    tcb *threadControlBlock = malloc(sizeof(tcb));
    threadControlBlock->currentContext = currentContext;
    threadControlBlock->threadContext = ucontext_thread;
    threadControlBlock->threadID = threadId++;
    *thread = threadControlBlock->threadID;
    threadControlBlock->status = 0; // Ready to be enqueued.

    if (threadCount == 0) {
        setupTimer();
    }

    if (SCHED) {
        threadControlBlock->threadPriority = -1; // Priority does not matter when using round-robin scheduling.
        normalEnqueue(readyQueue, threadControlBlock);
    } else {
        threadControlBlock->threadPriority = 0; // Priority is set to 0 when using PSJF scheduling.
        priorityEnqueue(readyQueue, threadControlBlock);
    }

    threadCount++;
    swapcontext(currentContext, scheduler_context);

    return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
    currentThreadControlBlock->status = 0; // 0 = ready
    if (getcontext(currentThreadControlBlock->currentContext) == -1) {
        perror("getcontext");
        return -1;
    }
    swapcontext(currentThreadControlBlock->currentContext, scheduler_context);
    return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
    currentThreadControlBlock->status = 3;
    // free any dynamic memory created by the thread
    free(currentThreadControlBlock->threadContext->uc_stack.ss_sp);
    free(currentThreadControlBlock->threadContext);
    free(currentThreadControlBlock->currentContext);
    free(currentThreadControlBlock);

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

        swapcontext(currentThreadControlBlock->currentContext, scheduler_context);

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

void setupTimer() {
    sigemptyset( &sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = &scheduler_interrupt_handler;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = QUANTUM; // 25ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = QUANTUM; // 25ms

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(-1);
    }
}

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


void freeCurrentThreadControlBlock() {
    free(currentThreadControlBlock->threadContext->uc_stack.ss_sp);
    free(currentThreadControlBlock->threadContext);
    free(currentThreadControlBlock->currentContext);
    free(currentThreadControlBlock);
}

void scheduler_interrupt_handler(int signalNumber) {
    // stop_timer();
    if (currentThreadControlBlock->status == 0) { // If the current thread is ready, we enqueue it
        // restart_timer();
        currentThreadControlBlock->status = 1;
        if (setcontext(currentThreadControlBlock->threadContext) == -1) {
            perror("setcontext");
            exit(-1);
        }
    }

    if (currentThreadControlBlock->status == 1 && signalNumber == SIGALRM) { // If the thread is new, we need to set it to ready
        currentThreadControlBlock->status = 0;
        if (SCHED) {
            normalEnqueue(readyQueue, currentThreadControlBlock);
        } else {
            currentThreadControlBlock->threadPriority++;
            priorityEnqueue(readyQueue, currentThreadControlBlock);
        }
    }

    tcb *nextThread = (tcb *) normalDequeue(readyQueue); // Get the next thread to run

    if (nextThread == NULL) {
        return;
    }

    freeCurrentThreadControlBlock();
    currentThreadControlBlock = nextThread; // Set the current thread to the next thread

    if (currentThreadControlBlock->status == 2) { // Enqueue it if it is blocked
        if (SCHED) {
            normalEnqueue(blockedQueue, currentThreadControlBlock);
        } else {
            priorityEnqueue(blockedQueue, currentThreadControlBlock);
        }
    }

    if (currentThreadControlBlock->status == 3) { // If the thread is finished, we need to free it
        nextThread = (tcb *) normalDequeue(readyQueue); // Get the next thread to run

        if (nextThread == NULL) {
            return;
        }

        freeCurrentThreadControlBlock();
        currentThreadControlBlock = nextThread; // Set the current thread to the next thread
    }
}


/* scheduler */
static void schedule() {
    if (currentThreadControlBlock == NULL) {
        currentThreadControlBlock = (tcb *) normalDequeue(readyQueue);

        if (currentThreadControlBlock == NULL) {
            return;
        }
    }

    scheduler_interrupt_handler(0);
    return;
}

/* The scheduler's context must be created before it can be switched into periodically*/
void create_schedule_context() {
    scheduler_context = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(scheduler_context);

    scheduler_context->uc_stack.ss_sp = malloc(SIGSTKSZ);
    scheduler_context->uc_stack.ss_size = SIGSTKSZ;
    scheduler_context->uc_stack.ss_flags = 0;

    // What should uc_link actually point to?
    scheduler_context->uc_link = default_context;


    makecontext(scheduler_context,schedule,0,NULL);
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
