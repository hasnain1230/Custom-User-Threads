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

int finishedThreads = 0;
pthread_t thread1,thread2,thread3,thread4;

void function1() {
    int i = 0;

    while(i < 1000000){
        i++;
    }

    printf("Thread 1 / Function 1: i = %d\n",i);
    pthread_exit(NULL);
    finishedThreads++;

    
}

void function2()
{
     int j = 0;

    while(j < 2000){
        j++;
    }   

    printf("Thread 2 / Function 2: j = %d\n",j);
    pthread_exit(NULL);
    finishedThreads++;
}


void function3()
{
    int k = 0;

    while(k < 1000000){
        k++;
    }
    printf("Thread 3 / Function 3: k = %d\n",k);
    pthread_exit(NULL);
    finishedThreads++;
}



void function4(){

    printf("Waiting on thread 3 / function 3\n");
    //pthread_join(thread3,NULL);
    printf("Thread 4 / Function 4 has joined");
    finishedThreads++;
    pthread_exit(NULL);

}

int main (void) {

    pthread_create(&thread1,NULL,(void*) function1,NULL);
    pthread_create(&thread2,NULL,(void*) function2,NULL);
    pthread_create(&thread3,NULL,(void*) function3,NULL);
    pthread_create(&thread4,NULL,(void*) function4,NULL);
    printf("------------------ %d\n",thread1);
    printf("------------------ %d\n",thread2);
    printf("------------------ %d\n",thread3);
    printf("------------------ %d\n",thread4);
    while(finishedThreads != 4){
        int i = 0;
    }

    return 0;
    
}

/*    pthread_t thread1,thread2,thread3,thread4;
    pthread_create(&thread1,NULL,(void*) function1,NULL);
    pthread_create(&thread2,NULL,(void*) function2,NULL);
    pthread_create(&thread3,NULL,(void*) function3,NULL);
    int  i = 0;
    while(1){
        if(i == 0){
            printf("%d\n",thread1);
            printf("%d\n",thread2);
            printf("%d\n",thread3);
        }
        i++;
    }
}
*/