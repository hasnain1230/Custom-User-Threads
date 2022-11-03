#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdbool.h>
#include "mypthread.h"

struct LinkedListNode { // Node for the linked list
    void *data;
    struct LinkedListNode *next;
};

struct LinkedList { // Pointer to the head and tail of the linked list as well as the current size.
    struct LinkedListNode *head, *tail;
    size_t currentSize;
};

bool isListEmpty(struct LinkedList *linkedList); // Checks if the linked list is empty
bool insert(struct LinkedList *linkedList, void *data); // Inserts a node at the end of the linked list
void *delete(struct LinkedList *linkedList, void *data); // Deletes the first node that matches the data
size_t getSize(struct LinkedList *linkedList);  // Returns the size of the linked list

#endif
