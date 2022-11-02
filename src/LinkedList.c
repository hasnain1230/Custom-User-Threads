//
// Created by lucidity on 11/2/22.
//

#include "LinkedList.h"
#include <stdlib.h>
#include <string.h>

bool isListEmpty(struct LinkedList *linkedList) {
    return linkedList->head == NULL;
}

bool insert(struct LinkedList *linkedList, void *data) {
    if (data == NULL) {
        return false;
    }
    // Insert new node at the end of the list
    struct LinkedListNode *newNode = malloc(sizeof(struct LinkedListNode));
    newNode->data = data;
    newNode->next = NULL;

    if (linkedList->head == NULL) {
        linkedList->head = newNode;
        linkedList->tail = newNode;
        linkedList->currentSize++;
    } else {
        linkedList->tail->next = newNode;
        linkedList->tail = newNode;
        linkedList->currentSize++;
    }

    return true;
}

void *delete(struct LinkedList *linkedList, void *data, size_t dataSize) {
    // Delete the first node that matches the data
    struct LinkedListNode *currentNode = linkedList->head;
    struct LinkedListNode *previousNode = NULL;

    while (currentNode != NULL) {
        if (currentNode->data == data) {
            if (previousNode == NULL) {
                // If the node to be deleted is the head
                linkedList->head = currentNode->next;
                linkedList->currentSize--;
                return currentNode;
            } else if (currentNode->next == NULL) {
                // If the node to be deleted is the tail
                linkedList->tail = previousNode;
                previousNode->next = NULL;
                linkedList->currentSize--;
                return currentNode;
            } else {
                // If the node to be deleted is in the middle
                previousNode->next = currentNode->next;
                linkedList->currentSize--;
                return currentNode;
            }
        }

        previousNode = currentNode;
        currentNode = currentNode->next;
    }

    return NULL;
}

size_t getSize(struct LinkedList *linkedList) {
    return linkedList->currentSize;
}
