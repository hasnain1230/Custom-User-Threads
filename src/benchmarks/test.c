#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <pthread.h>
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

void function1() {
    while(1){
        printf("Function 1 running\n");
        sleep(10);
    }
}

void function2()
{
    while(1) {
        printf("Function 2 running\n");
        sleep(10);
    }


}


void function3()
{
    while(1){
        printf("Function 3 running\n");
        sleep(10);
        
    }
}


void function4(){

    while(1){
        printf("Function 4 is working");
        sleep(10);
    }

}

int main (void) {

    atomic_bool lock;
    atomic_flag_clear(&lock);

    if (atomic_flag_test_and_set(&lock)) {
        printf("Lock is set\n");
    } else {
        printf("Lock is not set\n");
    }

    atomic_flag_test_and_set(&lock);

    if (lock) {
        printf("Lock is set\n");
    } else {
        printf("Lock is not set\n");
    }

    if (lock) {
        printf("Lock is set\n");
    } else {
        printf("Lock is not set\n");
    }


/*    pthread_t thread1,thread2,thread3,thread4;
    pthread_create(&thread1,NULL,(void*) function1,NULL);
    pthread_create(&thread2,NULL,(void*) function2,NULL);
    pthread_create(&thread3,NULL,(void*) function3,NULL);
    int  i = 0;
    while(1){
        i++;
        if(i == 100) {
            pthread_create(&thread4,NULL,(void*)function4,NULL);
            sleep(10000);
        }
    }*/
}
