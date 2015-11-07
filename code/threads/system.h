// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "synch.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "machine.h"
//#include <queue>

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

class AddrSpace;
class BitMap;
class Queue;

extern int currentTLBEntry;

extern bool isFIFO; //if not FIFO, RAND
extern Queue pageQueue;

extern OpenFile * swapFile;
extern char * swapFileName;
extern BitMap swapFileBM;

class IPTEntry : public TranslationEntry {
public:
	AddrSpace * space;
};

extern IPTEntry ipt[];
#ifdef USER_PROGRAM
#include "../userprog/bitmap.h"
#include "machine.h"
extern Machine* machine;
//-----BITMAP-----
extern BitMap bitmap;
extern Lock bitmapLock;


struct process {
	// AddrSpace * addrsp;
	int * threadStacks;
	int numThreadsRunning; // to check in exit if it's safe to kill the process
	int numThreadsTotal; // to know the index in the stack counter, i.e. the current address to add to
};

extern process * processTable[];
extern int processIndex;
extern int activeProcesses;
extern const int NUM_PROCESSES;
extern Lock processLock;
//---------------------------------------------
// LOCK SYSCALL
//---------------------------------------------
struct kernelLock {
	Lock * lock;
	AddrSpace * addrsp;
	bool isToBeDeleted;
};

extern kernelLock kernelLockList[];
extern int kernelLockIndex;
extern const int NUM_KERNEL_LOCKS;
extern Lock kernelLockLock;
//---------------------------------------------
// CONDITION SYSCALL
//---------------------------------------------
struct kernelCondition {
	Condition * condition;
	AddrSpace * addrsp;
	bool isToBeDeleted;
};

extern kernelCondition kernelConditionList[];
extern int kernelConditionIndex;
extern const int NUM_KERNEL_CONDITIONS;
extern Lock kernelConditionLock;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#define MAX_SIZE 101  //maximum size of the array that will store Queue.

// Creating a class named Queue.
class Queue
{
private:
	int A[MAX_SIZE];
	int front, rear;
public:
	// Constructor - set front and rear as -1.
	// We are assuming that for an empty Queue, both front and rear will be -1.
	Queue();

	// To check wheter Queue is empty or not
	bool isEmpty();

	// To check whether Queue is full or not
	bool isFull();

	// Inserts an element in queue at rear end
	void push(int x);

	// Removes an element in Queue from front end.
	int pop();
	// Returns element at front of queue.
	int Front();
	/*
	   Printing the elements in queue from front to rear.
	   This function is only to test the code.
	   This is not a standard function for Queue implementation.
	*/
	void Print();
};

#endif // SYSTEM_H
