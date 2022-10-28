// File:	mypthread.c

// List all group members' names:
// Hasnain Ali...
// iLab machine tested on:

#define _XOPEN_SOURCE
#include "mypthread.h"
#include <stdatomic.h>
#include <ucontext.h>
#include <assert.h>
#include <string.h>

#define STACKSIZE (2 * 1024 * 1024) // According to man pthread_attr_init, the default stack size is 2MB. Keeping with this, we'll initialize the stack size to 2MiB as well, converted to bytes.

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

void checkMalloc(void *ptr) {
    if (ptr == NULL) {
        perror("Malloc failed.");
        exit(1);
    }
}

struct Queue *initQueue() {
    struct Queue *queue = malloc(sizeof(struct Queue));
    queue->currentSize = 0;
    queue->head = NULL;
    queue->tail = NULL;

    return queue;
}

bool isEmpty(struct Queue *queue) {
    return queue->currentSize == 0;
}

void normalEnqueue(struct Queue *queue, tcb *threadControlBlock) {
    assert(threadControlBlock != NULL);
    struct Node *node = malloc(sizeof(struct Node));
    checkMalloc(node);

    node->data = threadControlBlock;
    node->dataSize = sizeof(tcb);
    node->next = NULL;

    if (isEmpty(queue)) {
        queue->head = queue->tail = node;
        queue->currentSize++;
        return;
    }

    queue->tail->next = node;
    node->prev = queue->tail;
    queue->tail = node;
    queue->currentSize++;
}

void *normalDequeue(struct Queue *queue) {
    if (isEmpty(queue)) {
        return NULL;
    }

    struct Node *nodeToDequeue = queue->head;
    void *dataToReturn = malloc(nodeToDequeue->dataSize);
    checkMalloc(dataToReturn);
    dataToReturn = memcpy(dataToReturn, nodeToDequeue->data, nodeToDequeue->dataSize);

    queue->head = queue->head->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    free(nodeToDequeue->data);
    free(nodeToDequeue);

    queue->currentSize--;

    return dataToReturn;
}

void preemptiveEnqueue(struct Queue *queue, tcb *threadControlBlock) {
    assert(threadControlBlock != NULL);

    if (isEmpty(queue)) {
        normalEnqueue(queue, threadControlBlock);
        return;
    }

    struct Node *ptr = queue->head;

    struct Node *node = malloc(sizeof(struct Node));
    checkMalloc(node);

    node->data = threadControlBlock;
    node->dataSize = sizeof(tcb);
    node->next = NULL;
    node->prev = NULL;

    uint threadPriority = threadControlBlock->threadPriority;

    if (threadPriority < ((tcb *) queue->head->data)->threadPriority) {
        node->next = queue->head;
        node->prev = NULL;
        queue->head->prev = node;
        queue->currentSize++;
        return;
    }

    while (ptr != NULL) {
        if (threadPriority < ((tcb *) ptr->data)->threadPriority) {
            node->next = ptr;
            node->prev = ptr->prev;
            ptr->prev->next = node;
            ptr->prev = node;
            queue->currentSize++;
            return;
        }

        ptr = ptr->next;
    }

    node->prev = queue->tail; // In case we need to enqueue at the end.
    queue->tail->next = node;
}



/*
void startFunction(void *(*function) (void*), void *args) {
    function(args);
}
*/

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function) (void*), void *arg) {
    static pid_t threadId = 0;
    ucontext_t *currentContext = malloc(sizeof(ucontext_t));
    ucontext_t *ucontext_thread = malloc(sizeof(ucontext_t));
    getcontext(currentContext);
    getcontext(ucontext_thread);
    ucontext_thread->uc_stack.ss_size = STACKSIZE;
    ucontext_thread->uc_stack.ss_sp = malloc(ucontext_thread->uc_stack.ss_size);
    ucontext_thread->uc_stack.ss_flags = 0;

    makecontext(ucontext_thread, (void *) function, 1, arg); // Might need a wrapper function?

    tcb *threadControlBlock = malloc(sizeof (tcb));
    threadControlBlock->currentContext = currentContext;
    threadControlBlock->threadContext = ucontext_thread;
    threadControlBlock->threadID = threadId++;
    threadControlBlock->isRunning = false;
    threadControlBlock->threadPriority = -1; //??? Probably need some helper function here to figure this out based on the ready queue.
	   // create a Thread Control Block
	   // create and initialize the context of this thread
	   // allocate heap space for this thread's stack
	   // after everything is all set, push this thread into the ready queue


	return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
	// YOUR CODE HERE
	
	// change current thread's state from Running to Ready
	// save context of this thread to its thread control block
	// switch from this thread's context to the scheduler's context

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	// YOUR CODE HERE

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
	// YOUR CODE HERE
	
	// each time a timer signal occurs your library should switch in to this context
	
	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF or MLFQ

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
