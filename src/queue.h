#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdio.h>
#include "mypthread.h"


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
void freeQueue(struct Queue *queue);

#endif QUEUE_H
