#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdio.h>
#include "mypthread.h"


struct Node { // Double linked list for the queue
    void *data;
    size_t dataSize;
    struct Node *prev, *next;
};

struct Queue { // Queue properties
    struct Node *head, *tail;
    size_t currentSize;
};

struct Queue *initQueue(); // Initializes the queue and returns a pointer to said queue.
bool isEmpty(struct Queue *queue); // Checks if Queue is empty
void normalEnqueue(struct Queue *queue, tcb *threadControlBlock); // Enqueues a thread control block without any priority
tcb *normalDequeue(struct Queue *queue); // Dequeues a thread control block. We just named it normalDequeue for clarity
void priorityEnqueue(struct Queue *queue, tcb *threadControlBlock); // Inserts the tcb into the queue based on priority
void freeQueue(struct Queue *queue); // Cleans up the queue.

#endif
