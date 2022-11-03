//
// Created by lucidity on 11/2/22.
//

#include "LinkedList.h"
#include <stdlib.h>
#include <string.h>

bool isListEmpty(struct LinkedList *linkedList) { // Checks if the linked list is empty
    return linkedList->head == NULL;
}

bool insert(struct LinkedList *linkedList, void *data) {
    if (data == NULL) {
        return false;
    }
    // Insert new node at the end of the list
    struct LinkedListNode *newNode = malloc(sizeof(struct LinkedListNode));
    newNode->data = data; // Store data in Node structure
    newNode->next = NULL;

    if (linkedList->head == NULL) { // If it's the first element we are inserting
        linkedList->head = newNode;
        linkedList->tail = newNode;
        linkedList->currentSize++;
    } else { // Otherwise just insert at the head.
        linkedList->tail->next = newNode;
        linkedList->tail = newNode;
        linkedList->currentSize++;
    }

    return true; // Return true if successful
}

void *delete(struct LinkedList *linkedList, void *data) {
    // Delete the first node that matches the data
    struct LinkedListNode *currentNode = linkedList->head; // Pointers to traverse done.
    struct LinkedListNode *previousNode = NULL;

    while (currentNode != NULL) {
        if (currentNode->data == data) { // We are comparing the memory addresses since we are not copying the data when we enqueue. The addresses will match.
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

        previousNode = currentNode; // Iterate pointers.
        currentNode = currentNode->next;
    }

    return NULL; // Found nothing to delete.
}

size_t getSize(struct LinkedList *linkedList) { // Returns the size of the linked list
    return linkedList->currentSize;
}
