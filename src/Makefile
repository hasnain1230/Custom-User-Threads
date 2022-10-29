CC = gcc
CFLAGS = -g -std=c99 -Wall -Wvla -Wextra -Wundef -Wshadow -Wpointer-arith -O2 -fexceptions -fsanitize=address,undefined
AR = ar -rcv
RANLIB = ranlib

SCHED = RR

all: queue.o mypthread.a

mypthread.a: mypthread.o
	$(AR) libmypthread.a mypthread.o # Generate an archive to link
	$(RANLIB) libmypthread.a # Generate an index table

mypthread.o: mypthread.h

queue.o: queue.h queue.c
	$(CC) $(CFLAGS) -c queue.c

ifeq ($(SCHED), RR)
	$(CC) -pthread $(CFLAGS) queue.o mypthread.c
else ifeq ($(SCHED), PSJF)
	$(CC) -pthread $(CFLAGS) -DPSJF queue.o mypthread.c
else ifeq ($(SCHED), MLFQ) # For the grad student class
	$(CC) -pthread $(CFLAGS) -DMLFQ mypthread.c
else
	echo "no such scheduling algorithm"
endif

clean:
	rm -rf testfile *.o *.a
