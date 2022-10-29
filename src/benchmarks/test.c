#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

/* A scratch program template on which to call and
 * test mypthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void repeat(){
	printf("Hello");
}


int main(int argc, char **argv) {


	//pthread_t threadID;
	//pthread_create(&threadID,NULL,(void*) repeat,NULL);

	create_schedule_context();
	setup_timer();
	while(1){
		
	}

	return 0;
}
