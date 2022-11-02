#include "queue.h"
#include "mypthread.h"
#include "checkMalloc.h"
#include <assert.h>
#include <string.h>


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
    node->prev = NULL;

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

tcb *normalDequeue(struct Queue *queue) {
    if (isEmpty(queue)) {
        return NULL;
    }

    struct Node *nodeToDequeue = queue->head;
    tcb *dataToReturn = malloc(nodeToDequeue->dataSize);
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

void priorityEnqueue(struct Queue *queue, tcb *threadControlBlock) {
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
        queue->head = node;
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

    queue->tail->next = node;
    node->prev = queue->tail;
    queue->tail = node;
    queue->currentSize++;
}

void freeQueue(struct Queue *queue) {
    struct Node *ptr = queue->head;

    while (ptr != NULL) {
        struct Node *temp = ptr;
        ptr = ptr->next;
        free(temp->data);
        free(temp);
    }

    free(queue);
}