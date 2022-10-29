CC = gcc
CFLAGS = -g -w

SCHED = RR

all:: parallel_cal vector_multiply external_cal test

parallel_cal:
	$(CC) $(CFLAGS) -pthread -o parallel_cal parallel_cal.c -L../ -lmypthread

vector_multiply:
	$(CC) $(CFLAGS) -pthread -o vector_multiply vector_multiply.c -L../ -lmypthread

external_cal:
	$(CC) $(CFLAGS) -pthread -o external_cal external_cal.c -L../ -lmypthread

test:
	$(CC) $(CFLAGS) -DRR -pthread -o test test.c -L../ -lmypthread

test_queue: ../queue.c test.c
	$(CC) -c $(CFLAGS) ../queue.c
	$(CC) -c $(CFLAGS) test.c
	$(CC) $(CFLAGS) queue.o test.o -o test_queue

clean:
	rm -rf testcase test parallel_cal vector_multiply external_cal *.o ./record/
