// File:	mypthread.c

// List all group members' names: Hasnain Ali, Rushabh Patel, Della Maret
// iLab machine tested on: rlab2.cs.rutgers.edu

#include "mypthread.h"
#include "queue.h"
#include "checkMalloc.h"
#include "LinkedList.h"
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
#define MAXTHREADS 128 // Maximum number of threads allowed

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

static struct sigaction sa1, sa2;
static struct itimerval timer;

static uint threadCount = 0;
static tcb *currentThreadControlBlock = NULL;
static tcb *mainThread = NULL;
static ucontext_t *scheduler_context, *default_context;
struct Queue *readyQueue = NULL;
static struct LinkedList *exitedThreads = NULL, *joinList = NULL;

void setupTimer();
void scheduler_interrupt_handler(int signalNumber);
static void sched_RR(int signalNumber);
static void sched_PSJF();
int mypthread_yield();
void restartTimer();

void cleanUp() {
    free(scheduler_context->uc_stack.ss_sp);
    free(scheduler_context);
    free(default_context->uc_stack.ss_sp);
    free(default_context);
    freeQueue(readyQueue);
}


/* create a new thread */
int mypthread_create(mypthread_t *thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
    static pid_t threadID = 0;
    if (threadCount == 0 && readyQueue == NULL && exitedThreads == NULL && joinList == NULL) { // If this is the first thread to be created, we need to initialize some structures.
        readyQueue = initQueue(); // Initialize the ready queue.

        exitedThreads = malloc(sizeof(struct LinkedList)); // Exit and waiting to join threads are stored in a linked list.
        exitedThreads->head = NULL;
        exitedThreads->tail = NULL;
        exitedThreads->currentSize = 0;

        joinList = malloc(sizeof(struct LinkedList));
        joinList->head = NULL;
        joinList->tail = NULL;
        joinList->currentSize = 0;

        create_schedule_context(); // Create the scheduler context to go to everytime the timer goes off or a new job is enqueued (depending on the algorithm)

        // atexit(cleanUp);
    }

    ucontext_t *currentContext = malloc(sizeof(ucontext_t));
    checkMalloc(currentContext);
    ucontext_t *ucontext_thread = malloc(sizeof(ucontext_t));
    checkMalloc(ucontext_thread);
    getcontext(ucontext_thread);
    getcontext(currentContext);

    if (getcontext(ucontext_thread) == -1 ^ getcontext(currentContext) == -1) { // This way, both contexts are evaluated and ensured they were gotten properly.
        perror("getcontext");

        free(currentContext);
        free(ucontext_thread);
        freeQueue(readyQueue);

        struct LinkedListNode *current = exitedThreads->head;

        while (current != NULL) {
            tcb *data = (tcb *) current->data;
            free(data);
            current = current->next;
        }

        free(exitedThreads);

        struct LinkedListNode *current2 = joinList->head;

        while (current2 != NULL) {
            tcb *data = (tcb *) current2->data;
            free(data->threadContext);
            free(data);
            current2 = current2->next;
        }

        free(current2);

        return -1;
    }

    ucontext_thread->uc_stack.ss_size = SIGSTKSZ;
    ucontext_thread->uc_stack.ss_sp = malloc(SIGSTKSZ);
    ucontext_thread->uc_link = currentContext;
    makecontext(ucontext_thread, (void *) function, 1, arg);

    tcb *threadControlBlock = malloc(sizeof(tcb));
    threadControlBlock->currentContext = currentContext;
    threadControlBlock->threadContext = ucontext_thread;
    threadControlBlock->threadID = threadID++;
    *thread = threadControlBlock->threadID;
    threadControlBlock->status = 0; // Ready to be enqueued.

    if (!SCHED) {
        threadControlBlock->threadPriority = -1; // Thread priority not important when using round-robin scheduling.
        normalEnqueue(readyQueue, threadControlBlock);

        if (threadCount == 0) {
            setupTimer();
        }

    } else {
        threadControlBlock->threadPriority = 0; // Thread priority is 0 when using PSJF scheduling.
        priorityEnqueue(readyQueue, threadControlBlock);
    }

    threadCount++;

    if (threadCount > MAXTHREADS) {
        perror("Maximum number of threads reached.");
        return -1;
    }


    swapcontext(currentContext, scheduler_context);

    return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield() {
    if (currentThreadControlBlock != NULL) {
        currentThreadControlBlock->status = 0; // 0 = ready
    }
    swapcontext(currentThreadControlBlock->threadContext, scheduler_context);
    return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
    if (value_ptr == NULL) {
        currentThreadControlBlock->returnValue = NULL;
    } else {
        currentThreadControlBlock->returnValue = value_ptr; // We are preserving the return value of the thread.
    }

    insert(exitedThreads, currentThreadControlBlock);

    currentThreadControlBlock->status = 3;
    // free any dynamic memory created by the thread
    free(currentThreadControlBlock->threadContext->uc_stack.ss_sp);
    free(currentThreadControlBlock->threadContext);
    free(currentThreadControlBlock->currentContext);

    threadCount--;

    struct LinkedListNode *current = joinList->head;

    while (current != NULL) {
        tcb *data = (tcb *) current->data;
        if (data->threadID == currentThreadControlBlock->threadID) {
            data->status = 1;

            if (!SCHED) {
                normalEnqueue(readyQueue, data);
            } else {
                data->threadPriority = 0;
                priorityEnqueue(readyQueue, data);
                mypthread_yield();
            }
            break;
        }
        current = current->next;
    }

    currentThreadControlBlock = normalDequeue(readyQueue);


    return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
    struct LinkedListNode *ptr = exitedThreads->head;

    tcb *joinThread = malloc(sizeof(tcb));
    joinThread->threadID = thread; // The TID of the threat we want to join.
    joinThread->threadContext = malloc(sizeof(ucontext_t));
    if (getcontext(joinThread->threadContext) == -1) {
        perror("getcontext");
        return -1;
    }

    while (ptr != NULL) {
        tcb *threadControlBlock = (tcb *) ptr->data;
        if (threadControlBlock->threadID == thread) {
            if (value_ptr != NULL) {
                *value_ptr = threadControlBlock->returnValue;
            }

            delete(exitedThreads, threadControlBlock, sizeof(tcb));
            free(threadControlBlock);
            free(ptr);
            free(joinThread->threadContext);
            free(joinThread);
            return 0;
        }
        ptr = ptr->next;
    }

    joinThread->status = 2; // 2 = waiting for the thread to exit
    insert(joinList, joinThread);

    // wait for a specific thread to terminate
    // deallocate any dynamic memory created by the joining thread

    return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    assert(mutexattr == NULL); // We don't support mutex attributes

    atomic_flag_clear(&mutex->lock);
    mutex->owner = currentThreadControlBlock->threadID;
    mutex->waitingQueue = (struct Queue *) initQueue();

    return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
    if (atomic_flag_test_and_set(&mutex->lock)) {
        // Lock is already set, so we need to block the thread
        currentThreadControlBlock->status = 2;

        if (!SCHED) {
            normalEnqueue(mutex->waitingQueue, currentThreadControlBlock);
        } else {
            currentThreadControlBlock->threadPriority = 0; // We set the priority to 0 so that it is the first to be scheduled when it is unblocked. If they have the same priority, it is first come first serve.
            priorityEnqueue(mutex->waitingQueue, currentThreadControlBlock);
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
    if (currentThreadControlBlock->threadID == mutex->owner) {
        atomic_flag_clear(&mutex->lock);

        struct Queue *waitingQueue = (struct Queue *) mutex->waitingQueue;

        if (!isEmpty(waitingQueue)) {
            tcb *threadControlBlock = normalDequeue(waitingQueue);
            threadControlBlock->status = 0;

            if (!SCHED) {
                normalEnqueue(readyQueue, threadControlBlock);
            } else {
                threadControlBlock->threadPriority = 0;
                priorityEnqueue(readyQueue, threadControlBlock);
                swapcontext(currentThreadControlBlock->currentContext, scheduler_context);
            }
        }

        return 0;
    } else {
        return -1;
    }
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
    freeQueue(mutex->waitingQueue);
    free(mutex);
    return 0;
};

void swapToScheduler() {
    swapcontext(currentThreadControlBlock->threadContext, scheduler_context);
}

void setupTimer() {
    sigemptyset( &sa1.sa_mask);
    sa1.sa_flags = SA_RESTART;
    sa1.sa_handler = &swapToScheduler;
    sigaction(SIGALRM, &sa1, NULL);

    sigemptyset( &sa2.sa_mask);
    sa2.sa_flags = SA_RESTART;
    sa2.sa_handler = &swapToScheduler;
    sigaction(SIGVTALRM, &sa2, NULL);;

    restartTimer(); // We are technically starting the timer here, but it is effectively the same as restarting it.
}

/* setup new interval timer*/
/* After finishing running library code, restart timer to allow scheduler to run periodically*/
void restartTimer() {
    memset(&timer, 0, sizeof(timer));
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = QUANTUM; // 25ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = QUANTUM; // 25ms

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(-1);
    }
}

/* Before running any library code, stop the timer so that library code does not get interrupt*/
void stopTimer() {
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
    if (!SCHED) { // Means we are using round robin
        sched_RR(signalNumber);
        return;
    } else {
        sched_PSJF();
        return;
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
    // scheduler_context->uc_link = default_context;


    makecontext(scheduler_context,schedule,0,NULL);
}

/* Round Robin scheduling algorithm */
static void sched_RR(int signalNumber) {
    stopTimer();
    if (currentThreadControlBlock->status == 0) {
        currentThreadControlBlock->status = 1;
        restartTimer();
        if (setcontext(currentThreadControlBlock->threadContext) == -1) {
            perror("setcontext");
            exit(-1);
        }
    }


    if (currentThreadControlBlock->status == 1 /*&& (signalNumber == SIGALRM || signalNumber == SIGVTALRM)*/) { // If the thread is new, we need to set it to ready
        currentThreadControlBlock->status = 0;

        normalEnqueue(readyQueue, currentThreadControlBlock);
        tcb *nextThread = (tcb *) normalDequeue(readyQueue); // Get the next thread to run
        currentThreadControlBlock = nextThread; // Set the current thread to the next thread

        if (!isEmpty(readyQueue)) {
            exit(1);
        }

        restartTimer();

        if (setcontext(currentThreadControlBlock->threadContext) == -1) {
            perror("setcontext");
            exit(-1);
        }
    }
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF() {
    if (currentThreadControlBlock->status == 0) {
        currentThreadControlBlock->status = 1;
        if (setcontext(currentThreadControlBlock->threadContext) == -1) {
            perror("setcontext");
            exit(-1);
        }
    } else if (currentThreadControlBlock->status == 1) { // We've already checked at this point that this thread has a greater priority than the currently running thread.
        currentThreadControlBlock->status = 0;
        currentThreadControlBlock->threadPriority++; // Increment the priority of the thread that just ran
        priorityEnqueue(readyQueue, currentThreadControlBlock);
        tcb *nextThread = normalDequeue(readyQueue); // Get the next thread to run
        currentThreadControlBlock = nextThread; // Set the current thread to the next thread
        currentThreadControlBlock->status = 1;

        if (setcontext(currentThreadControlBlock->threadContext) == -1) {
            perror("setcontext");
            exit(-1);
        }
    }
}
