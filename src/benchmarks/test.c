#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "../mypthread.h"
#include "../queue.h"

#ifndef PSJF
#define SCHED 0 // Indicates round-robin scheduling
#else
#define SCHED 1 // Indicates PSJF scheduling
#endif

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

int main (void) {
    if (SCHED == 0) {
        printf("Round-robin scheduling\n");
    } else {
        printf("PSJF scheduling\n");
    }
}
