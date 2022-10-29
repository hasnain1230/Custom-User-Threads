//
// Created by lucidity on 10/29/22.
//

#include <stdbool.h>
#include "mypthread.h"

#ifndef CUSTOM_USER_THREADS_QUEUE_H
#define CUSTOM_USER_THREADS_QUEUE_H

struct Node {
    void *data;
    size_t dataSize;
    struct Node *prev, *next;
};

struct Queue {
    struct Node *head, *tail;
    size_t currentSize;
};

struct Queue *initQueue();
bool isEmpty(struct Queue *queue);
void normalEnqueue(struct Queue *queue, tcb *threadControlBlock);
tcb *normalDequeue(struct Queue *queue);
void priorityEnqueue(struct Queue *queue, tcb *threadControlBlock);

#endif //CUSTOM_USER_THREADS_QUEUE_H
