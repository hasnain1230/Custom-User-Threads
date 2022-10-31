CC = gcc
CFLAGS = -g
AR = ar -rcv
RANLIB = ranlib

SCHED = RR

all: mypthread.a

mypthread.a: mypthread.o # Generate an archive to link and Generate an index table
	$(AR) libmypthread.a queue.o checkMalloc.o mypthread.o
	$(RANLIB) libmypthread.a

mypthread.o: mypthread.h

ifeq ($(SCHED), RR)
	$(CC) -pthread -c $(CFLAGS) queue.c mypthread.c checkMalloc.c
else ifeq ($(SCHED), PSJF)
	$(CC) -pthread -c $(CFLAGS) -DPSJF queue.c mypthread.c
else ifeq ($(SCHED), MLFQ) # For the grad student class
	$(CC) -pthread $(CFLAGS) -DMLFQ mypthread.c
else
	echo "no such scheduling algorithm"
endif

clean:
	rm -rf testfile *.o *.a
