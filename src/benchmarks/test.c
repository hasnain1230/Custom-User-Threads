#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "../mypthread.h"
#include "../queue.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

int main (void) {
    tcb *threadControlBlock = malloc(sizeof(tcb));
    threadControlBlock->isRunning = true;
    threadControlBlock->threadID = 1;
    threadControlBlock->threadPriority = 40;
    threadControlBlock->currentContext = NULL;
    threadControlBlock->threadContext = NULL;

    tcb *threadControlBlock1 = malloc(sizeof(tcb));
    threadControlBlock1->isRunning = true;
    threadControlBlock1->threadID = 2;
    threadControlBlock1->threadPriority = 20;
    threadControlBlock1->currentContext = NULL;
    threadControlBlock1->threadContext = NULL;


    struct Queue *queue = initQueue();
    priorityEnqueue(queue, threadControlBlock);

    tcb *tcb1 = (tcb *) normalDequeue(queue);
    printf("Thread ID: %d\n", tcb1->threadPriority);


    free(queue);
}
