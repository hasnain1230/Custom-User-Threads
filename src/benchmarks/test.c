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
    puts("function1");
    pthread_exit(NULL);
}

void function2()
{
    puts("function2");
    pthread_exit(NULL);
}


void function3()
{
    puts("function3");
    pthread_exit(NULL);
}


void function4(){

    puts("function4");
    pthread_exit(NULL);

}

int main (void) {

    pthread_t thread1,thread2,thread3,thread4;
    pthread_create(&thread1,NULL,(void*) function1,NULL);
    pthread_create(&thread2,NULL,(void*) function2,NULL);
    pthread_create(&thread3,NULL,(void*) function3,NULL);
    pthread_create(&thread4,NULL,(void*) function4,NULL);
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