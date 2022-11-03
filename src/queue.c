#include "queue.h"
#include "mypthread.h"
#include "checkMalloc.h"
#include <assert.h>
#include <string.h>


struct Queue *initQueue() { // Initializes the queue and returns a pointer to said queue.
    struct Queue *queue = malloc(sizeof(struct Queue));
    queue->currentSize = 0;
    queue->head = NULL;
    queue->tail = NULL;

    return queue;
}

bool isEmpty(struct Queue *queue) { // Checks if Queue is empty
    return queue->currentSize == 0;
}

void normalEnqueue(struct Queue *queue, tcb *threadControlBlock) { // Enqueues a thread control block without any priority
    assert(threadControlBlock != NULL);
    struct Node *node = malloc(sizeof(struct Node));
    checkMalloc(node);

    node->data = threadControlBlock; // Creating node to enqueue.
    node->dataSize = sizeof(tcb);
    node->next = NULL;
    node->prev = NULL;

    if (isEmpty(queue)) { // Inserting into head position
        queue->head = queue->tail = node;
        queue->currentSize++;
        return;
    }

    queue->tail->next = node; // Inserting into queue after head.
    node->prev = queue->tail;
    queue->tail = node;
    queue->currentSize++;
}

tcb *normalDequeue(struct Queue *queue) { // Dequeues a thread control block. We just named it normalDequeue for clarity
    if (isEmpty(queue)) {
        return NULL; // If queue is empty, return NULL
    }

    struct Node *nodeToDequeue = queue->head; // Creating node to dequeue
    tcb *dataToReturn = malloc(nodeToDequeue->dataSize); // Creating the data to return
    checkMalloc(dataToReturn);
    dataToReturn = memcpy(dataToReturn, nodeToDequeue->data, nodeToDequeue->dataSize); // Copy the memory over to our return.

    queue->head = queue->head->next; // Remove from head

    if (queue->head == NULL) {
        queue->tail = NULL; // List is empty
    }

    free(nodeToDequeue->data); // Clean up
    free(nodeToDequeue);

    queue->currentSize--;

    return dataToReturn;
}

void priorityEnqueue(struct Queue *queue, tcb *threadControlBlock) {
    assert(threadControlBlock != NULL);

    if (isEmpty(queue)) { // If queue is empty, just enqueue normally.
        normalEnqueue(queue, threadControlBlock);
        return;
    }

    struct Node *ptr = queue->head;

    struct Node *node = malloc(sizeof(struct Node)); // Creating node to enqueue.
    checkMalloc(node);

    node->data = threadControlBlock; // Setting data
    node->dataSize = sizeof(tcb);
    node->next = NULL;
    node->prev = NULL;

    uint threadPriority = threadControlBlock->threadPriority;

    if (threadPriority < ((tcb *) queue->head->data)->threadPriority) { // If the thread priority is less than the head, insert at head because the current priority is the highest. Lower means greater priority.
        node->next = queue->head;
        node->prev = NULL;
        queue->head->prev = node;
        queue->head = node;
        queue->currentSize++;
        return;
    }

    while (ptr != NULL) { // Insert in the middle of the queue
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

    queue->tail->next = node; // Insert at the end, this thread is very little priority.
    node->prev = queue->tail;
    queue->tail = node;
    queue->currentSize++;
}

void freeQueue(struct Queue *queue) { // Cleans up the queue.
    struct Node *ptr = queue->head;

    while (ptr != NULL) {
        struct Node *temp = ptr;
        ptr = ptr->next;
        free(temp->data);
        free(temp);
    }

    free(queue);
}