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

#ifndef PSJF
#define SCHED 0 // Indicates round-robin scheduling
#else
#define SCHED 1 // Indicates PSJF scheduling
#endif

#define QUANTUM 6000 // Quantum is set to 6ms = 6000 us
#define MAXTHREADS 128 // Maximum number of threads allowed

static struct sigaction sa1, sa2; // Signal handlers
static struct itimerval timer; // Timer for Round Robin scheduling

static uint threadCount = 0;
static tcb *currentThreadControlBlock = NULL; // Current thread control block
static ucontext_t *scheduler_context;
struct Queue *readyQueue = NULL;
static struct LinkedList *exitedThreads = NULL, *joinList = NULL; // Two linked lists, one for the threads that have existed, and one for the threads that are waiting to be joined.

void setupTimer();
void scheduler_interrupt_handler();
static void sched_RR();
static void sched_PSJF();
int mypthread_yield();
void restartTimer();

/* create a new thread and push it on to the queue */
int mypthread_create(mypthread_t *thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
    static pid_t threadID = 0; // Static because we want to retain the value for the lifetime of the primary execution context.
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
    }

    ucontext_t *currentContext = malloc(sizeof(ucontext_t));
    checkMalloc(currentContext);
    ucontext_t *ucontext_thread = malloc(sizeof(ucontext_t));
    checkMalloc(ucontext_thread);

    if (getcontext(ucontext_thread) == -1 ^ getcontext(currentContext) == -1) { // This way, both contexts are evaluated and ensured they were gotten properly.
        perror("getcontext");

        free(currentContext);
        free(ucontext_thread);
        freeQueue(readyQueue);

        struct LinkedListNode *current = exitedThreads->head;

        // Clean up everything that may be allocated in case of an error or something went wrong when getting the context.
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

        return -1; // Error creating the pthread.
    }

    ucontext_thread->uc_stack.ss_size = SIGSTKSZ; // Default stack size in Linux Operating Systems
    ucontext_thread->uc_stack.ss_sp = malloc(SIGSTKSZ);
    ucontext_thread->uc_link = currentContext;
    makecontext(ucontext_thread, (void *) function, 1, arg); // Allows us to context switch into the function.

    // Create the actual thread control block.
    tcb *threadControlBlock = malloc(sizeof(tcb));
    threadControlBlock->currentContext = currentContext;
    threadControlBlock->threadContext = ucontext_thread;
    threadControlBlock->threadID = threadID++;
    *thread = threadControlBlock->threadID;
    threadControlBlock->status = 0; // Ready to be enqueued.

    if (!SCHED) { // Indicates Round Robin Scheduling Algorithm
        threadControlBlock->threadPriority = -1; // Thread priority not important when using round-robin scheduling.
        normalEnqueue(readyQueue, threadControlBlock); // Round Robin does not require priority scheduling

        if (threadCount == 0) {
            setupTimer(); // Since we are using round-robin, we need to set up the timer.
        }

    } else {
        threadControlBlock->threadPriority = 0; // Thread priority is 0 when using PSJF scheduling.
        priorityEnqueue(readyQueue, threadControlBlock); // We need to enqueue in the right place in the queue based on the current priority.
    }

    threadCount++; // Increment the thread count.

    if (threadCount > MAXTHREADS) {
        perror("Maximum number of threads reached.");
        return -1;
    }


    swapcontext(currentContext, scheduler_context); // Save the current context and go to the scheduler context.

    return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield() {
    if (currentThreadControlBlock != NULL) {
        currentThreadControlBlock->status = 0; // 0 = ready
    }
    swapcontext(currentThreadControlBlock->threadContext, scheduler_context); // Save the current context and surrender its running time.
    return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
    if (value_ptr == NULL) { // Checking if there is a return value.
        currentThreadControlBlock->returnValue = NULL;
    } else {
        currentThreadControlBlock->returnValue = value_ptr; // We are preserving the return value of the thread.
    }

    insert(exitedThreads, currentThreadControlBlock); // Insert the thread into the exited threads linked list.

    currentThreadControlBlock->status = 3;  // Indicate termination.
    // free any dynamic memory created by the thread
    free(currentThreadControlBlock->threadContext->uc_stack.ss_sp);
    free(currentThreadControlBlock->threadContext);
    free(currentThreadControlBlock->currentContext);

    threadCount--; // Thread has exited, so decrement thread count.

    struct LinkedListNode *current = joinList->head; // Check if any threads are waiting to be joined.

    while (current != NULL) {
        tcb *data = (tcb *) current->data;
        if (data->threadID == currentThreadControlBlock->threadID) {
            data->status = 0; // Ready to be enqueued and joined.

            if (!SCHED) {
                normalEnqueue(readyQueue, data); // Round robin scheduling.
            } else {
                data->threadPriority = 0; // Set priority and enqueue
                priorityEnqueue(readyQueue, data);
                mypthread_yield(); // Trap out of the context and let the recently unblocked thread run.
            }
            break; // If we found it, then unblock it; we are done.
        }
        current = current->next;
    }

    currentThreadControlBlock = normalDequeue(readyQueue);// The current TCB is done. Get the next one to be the currently running thread.

    return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
    struct LinkedListNode *ptr = exitedThreads->head; // Checking to see which threads have exited.

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
                *value_ptr = threadControlBlock->returnValue; // Preserving the return value.
            }

            delete(exitedThreads, threadControlBlock); // Delete this thread from the join list.
            free(threadControlBlock);
            free(ptr);
            free(joinThread->threadContext);
            free(joinThread);
            return 0; // Clean up and return.
        }
        ptr = ptr->next;
    }

    joinThread->status = 2; // 2 = waiting for the thread to exit
    insert(joinList, joinThread); // Wait for thread to exit before we can join.

    return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
    atomic_flag_clear(&mutex->lock); // Clear the lock.
    mutex->waitingQueue = (struct Queue *) initQueue(); // Initialize the waiting queue for this specific mutex.

    return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex) {
    if (atomic_flag_test_and_set(&mutex->lock)) {
        // Lock is already set, so we need to block the thread
        currentThreadControlBlock->status = 2; // This current thread is locked. It is waiting for the mutex to be unlocked.

        if (!SCHED) {
            normalEnqueue(mutex->waitingQueue, currentThreadControlBlock); // Place them in a block queue for threads waiting to gain access.
        } else {
            currentThreadControlBlock->threadPriority = 0; // We set the priority to 0 so that it is the first to be scheduled when it is unblocked. If they have the same priority, it is first come first serve.
            priorityEnqueue(mutex->waitingQueue, currentThreadControlBlock);
        }

        swapcontext(currentThreadControlBlock->currentContext, scheduler_context); // Swap to the next context because this one is blocked on a mutex.

        return -1;
    } else {
        mutex->owner = currentThreadControlBlock->threadID; // This thread owns the lock. He and he own is powerful enough to hold such power. No one else is strong enough!
        return 0;
    }
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex) {
    if (currentThreadControlBlock->threadID == mutex->owner) { // Only the owner can unlock the mutex.
        atomic_flag_clear(&mutex->lock);

        struct Queue *waitingQueue = (struct Queue *) mutex->waitingQueue;

        if (!isEmpty(waitingQueue)) {
            tcb *threadControlBlock = normalDequeue(waitingQueue);
            threadControlBlock->status = 0; // This thread is ready to be enqueued and run.

            if (!SCHED) { // Round Robin Scheduling
                normalEnqueue(readyQueue, threadControlBlock);
            } else { // Preemptive Priority Scheduling
                threadControlBlock->threadPriority = 0;
                priorityEnqueue(readyQueue, threadControlBlock);
                swapcontext(currentThreadControlBlock->currentContext, scheduler_context);
            }
        }

        return 0;
    } else {
        return -1; // This thread does not own the lock, so it cannot unlock it.
    }
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex) {
    freeQueue(mutex->waitingQueue); // Free the wait queue.
    return 0;
};

void swapToScheduler() {
    if (currentThreadControlBlock == NULL && isEmpty(readyQueue)) { // No more work left to do, so we return back to default context.
        return;
    }
    swapcontext(currentThreadControlBlock->threadContext, scheduler_context); // Swap to the scheduler context. There is more work left to do.
}

void setupTimer() { // Set up the timer and signal handler for SIGALRM and SIGVTALRM.
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
    timer.it_value.tv_usec = QUANTUM; // 6ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = QUANTUM; // 6ms

    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(-1);
    }
}

/* Before running any library code, stop the timer so that library code does not get interrupt*/
void stopTimer() { // Stop the timer.
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL,&timer,NULL);
}


void scheduler_interrupt_handler() {
    if (!SCHED) { // Means we are using round robin
        sched_RR();  // Go to RR scheduler
        return;
    } else {
        sched_PSJF(); // Go to PSJF scheduler
        return;
    }
}


/* scheduler */
static void schedule() { // Allow us get the next thread if the current one is null
    if (currentThreadControlBlock == NULL) {
        currentThreadControlBlock = (tcb *) normalDequeue(readyQueue);

        if (currentThreadControlBlock == NULL) {
            return;
        }
    }

    scheduler_interrupt_handler(); // Call the scheduler interrupt handler for the appropriate scheduler.
    return;
}

/* The scheduler's context must be created before it can be switched into periodically*/
void create_schedule_context() { // Create the scheduler context so we can come back to it anytime.
    scheduler_context = (ucontext_t*) malloc(sizeof(ucontext_t));

    getcontext(scheduler_context);

    scheduler_context->uc_stack.ss_sp = malloc(SIGSTKSZ); // Default stack size in Linux.
    scheduler_context->uc_stack.ss_size = SIGSTKSZ;
    scheduler_context->uc_stack.ss_flags = 0;

    // What should uc_link actually point to?
    // scheduler_context->uc_link = default_context;


    makecontext(scheduler_context,schedule,0,NULL);
}

/* Round Robin scheduling algorithm */
static void sched_RR() {
    stopTimer();
    if (currentThreadControlBlock->status == 0) { // This thread is ready to be enqueued and run.
        currentThreadControlBlock->status = 1; // We start running the thread
        restartTimer(); // Restart the timer so that the thread can run for QUANTUM amount of time.
        if (setcontext(currentThreadControlBlock->threadContext) == -1) {
            perror("setcontext");
            exit(-1);
        }
    }


    if (currentThreadControlBlock->status == 1) { // If the thread is new, we need to set it to ready
        currentThreadControlBlock->status = 0;

        normalEnqueue(readyQueue, currentThreadControlBlock);
        tcb *nextThread = (tcb *) normalDequeue(readyQueue); // Get the next thread to run
        currentThreadControlBlock = nextThread; // Set the current thread to the next thread
        currentThreadControlBlock->status = 1; // Set the status of the next thread to running

        restartTimer(); // Restart timer and swap to the next thread

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
        if (setcontext(currentThreadControlBlock->threadContext) == -1) { // Run the thread with the highest priority right now.
            perror("setcontext");
            exit(-1);
        }
    } else if (currentThreadControlBlock->status == 1) { // We've already checked at this point that this thread has a greater priority than the currently running thread.
        currentThreadControlBlock->status = 0;
        currentThreadControlBlock->threadPriority++; // Increment the priority of the thread that just ran
        priorityEnqueue(readyQueue, currentThreadControlBlock);
        tcb *nextThread = normalDequeue(readyQueue); // Get the next thread to run
        currentThreadControlBlock = nextThread; // Set the current thread to the next thread
        currentThreadControlBlock->status = 1; // Set the status of the next thread to running

        if (setcontext(currentThreadControlBlock->threadContext) == -1) { // Context switch into next thread
            perror("setcontext");
            exit(-1);
        }
    }
}
