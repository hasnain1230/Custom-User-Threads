#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdbool.h>
#include "mypthread.h"

struct LinkedListNode {
    void *data;
    struct LinkedListNode *next;
};

struct LinkedList {
    struct LinkedListNode *head, *tail;
    size_t currentSize;
};

bool isListEmpty(struct LinkedList *linkedList);
bool insert(struct LinkedList *linkedList, void *data);
void *delete(struct LinkedList *linkedList, void *data, size_t dataSize);
size_t getSize(struct LinkedList *linkedList);


#endif
