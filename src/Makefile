CC = gcc
CFLAGS = -g -c -std=c99 -Wall -Wvla -Wextra -Wundef -Wshadow -Wpointer-arith -O2 -fexceptions -fsanitize=address,undefined
AR = ar -rcv
RANLIB = ranlib

SCHED = RR

all: mypthread.a

mypthread.a: mypthread.o
	$(AR) libmypthread.a mypthread.o # Generate an archive to link
	$(RANLIB) libmypthread.a # Generate an index table

mypthread.o: mypthread.h

ifeq ($(SCHED), RR)
	$(CC) -pthread $(CFLAGS) mypthread.c
else ifeq ($(SCHED), PSJF)
	$(CC) -pthread $(CFLAGS) -DPSJF mypthread.c
else ifeq ($(SCHED), MLFQ) # For the grad student class
	$(CC) -pthread $(CFLAGS) -DMLFQ mypthread.c
else
	echo "no such scheduling algorithm"
endif

clean:
	rm -rf testfile *.o *.a
