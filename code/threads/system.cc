// system.cc 
//	Nachos initialization and cleanup routines.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// This defines *all* of the global data structures used by Nachos.
// These are all initialized and de-allocated by this file.

Thread *currentThread;			// the thread we are running now
Thread *threadToBeDestroyed;  		// the thread that just finished
Scheduler *scheduler;			// the ready list
Interrupt *interrupt;			// interrupt status
Statistics *stats;			// performance metrics
Timer *timer;				// the hardware timer device,
// for invoking context switches

bool isFIFO = true; //assumption: if -P option is not used we want FIFO
int machineNum = -1; //until formally set in Initialize
Queue pageQueue;

int currentTLBEntry = 0;
IPTEntry ipt[NumPhysPages];
Lock iptLock("IPT lock");
Lock pageTableLock("Page table lock");

OpenFile * swapFile = NULL;
char * swapFileName = "../swapfile";
BitMap swapFileBM(10000000);
Lock swapLock("Swap file lock");

//----- NEW PASSPORT OFFICE SHIT ----------
//class PassportOffice : PostOffice {
	// not sure if we need this i have something vague in my notes
//};

//-----------------------------------------

#ifdef USER_PROGRAM	// requires either FILESYS or FILESYS_STUB
Machine *machine;	// user program memory and registers
BitMap bitmap(NumPhysPages);
Lock bitmapLock("Bitmap lock");

//---------------------------------------------
// PROCESS TABLE
//---------------------------------------------
const int NUM_PROCESSES = 10;
process * processTable[NUM_PROCESSES];
int processIndex = 0;
int activeProcesses = 0;
Lock processLock("Process Lock");
//---------------------------------------------
// THREAD SYSCALL
//---------------------------------------------
const int NUM_KERNEL_LOCKS = 1000;
kernelLock kernelLockList[NUM_KERNEL_LOCKS];
int kernelLockIndex = 0;
Lock kernelLockLock("Kernel Locks Lock ");
//---------------------------------------------
// CONDITION SYSCALL
//---------------------------------------------
const int NUM_KERNAL_CONDITIONS = 1000;
kernelCondition kernelConditionList[NUM_KERNAL_CONDITIONS];
int kernelConditionIndex = 0;
Lock kernelConditionLock("Kernel Conditions Lock");

const int NUM_KERNEL_MONITORS = 1000;
kernelMonitor kernelMonitorList[NUM_KERNEL_MONITORS];
int kernelMonitorIndex = 0;
Lock kernelMonitorLock("Kernel Monitors Lock");

int NUM_SERVERS = 0;
vector<Request> requests;
#endif

#ifdef FILESYS_NEEDED
FileSystem *fileSystem;
#endif

#ifdef FILESYS
SynchDisk *synchDisk;
#endif

#ifdef NETWORK
PostOffice *postOffice;
#endif

// External definition, to allow us to take a pointer to this function
extern void Cleanup();

//----------------------------------------------------------------------
// TimerInterruptHandler
// 	Interrupt handler for the timer device.  The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	"dummy" is because every interrupt handler takes one argument,
//		whether it needs it or not.
//----------------------------------------------------------------------
static void TimerInterruptHandler(int dummy) {
	if (interrupt->getStatus() != IdleMode)
		interrupt->YieldOnReturn();
}

//----------------------------------------------------------------------
// Initialize
// 	Initialize Nachos global data structures.  Interpret command
//	line arguments in order to determine flags for the initialization.  
// 
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
void Initialize(int argc, char **argv) {
	int argCount;
	char* debugArgs = "";
	bool randomYield = FALSE;

#ifdef USER_PROGRAM
	bool debugUserProg = FALSE;	// single step user program
#endif
#ifdef FILESYS_NEEDED
	bool format = FALSE;	// format disk
#endif
#ifdef NETWORK
	double rely = 1;		// network reliability
	int netname = 0;// UNIX socket name
#endif

	for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
		argCount = 1;
		if (!strcmp(*argv, "-d")) {
			if (argc == 1)
				debugArgs = "+";	// turn on all debug flags
			else {
				debugArgs = *(argv + 1);
				argCount = 2;
			}
		}
		else if (!strcmp(*argv, "-rs")) {
			ASSERT(argc > 1);
			RandomInit(atoi(*(argv + 1)));	// initialize pseudo-random
			// number generator
			randomYield = TRUE;
			argCount = 2;
		}
#ifdef USER_PROGRAM
		if (!strcmp(*argv, "-s"))
		debugUserProg = TRUE;
#endif
#ifdef FILESYS_NEEDED
		if (!strcmp(*argv, "-f"))
		format = TRUE;
#endif
#ifdef NETWORK
	if (!strcmp(*argv, "-l")) {
	    ASSERT(argc > 1);
	    rely = atof(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-m")) {
	    ASSERT(argc > 1);
	    netname = atoi(*(argv + 1));
        machineNum = netname;
	    argCount = 2;
	}
#endif
	}

	DebugInit(debugArgs);			// initialize DEBUG messages
	stats = new Statistics();			// collect statistics
	interrupt = new Interrupt;			// start up interrupt handling
	scheduler = new Scheduler();		// initialize the ready queue
	if (randomYield)				// start the timer (if needed)
		timer = new Timer(TimerInterruptHandler, 0, randomYield);

	threadToBeDestroyed = NULL;

	// We didn't explicitly allocate the current thread we are running in.
	// But if it ever tries to give up the CPU, we better have a Thread
	// object to save its state.
	currentThread = new Thread("main", -1); //giving this -1 because it should never be used as an index for a process
	currentThread->setStatus(RUNNING);

	interrupt->Enable();
	CallOnUserAbort(Cleanup);			// if user hits ctl-C

#ifdef USER_PROGRAM
	machine = new Machine(debugUserProg);	// this must come first
#endif

#ifdef FILESYS
	synchDisk = new SynchDisk("DISK");
#endif

#ifdef FILESYS_NEEDED
	fileSystem = new FileSystem(format);
#endif

#ifdef NETWORK
	postOffice = new PostOffice(netname, rely, 10);
#endif
}

//----------------------------------------------------------------------
// Cleanup
// 	Nachos is halting.  De-allocate global data structures.
//----------------------------------------------------------------------
void Cleanup() {
	printf("\nCleaning up...\n");
#ifdef NETWORK
	delete postOffice;
#endif

#ifdef USER_PROGRAM
	delete machine;
#endif

#ifdef FILESYS_NEEDED
	delete fileSystem;
#endif

#ifdef FILESYS
	delete synchDisk;
#endif

	delete timer;
	delete scheduler;
	delete interrupt;

	Exit(0);
}

// Constructor - set front and rear as -1.
// We are assuming that for an empty Queue, both front and rear will be -1.
Queue::Queue() {
	front = -1;
	rear = -1;
}

// To check wheter Queue is empty or not
bool Queue::isEmpty() {
	return (front == -1 && rear == -1);
}

// To check whether Queue is full or not
bool Queue::isFull() {
	return (rear + 1) % MAX_SIZE == front ? true : false;
}

// Inserts an element in queue at rear end
void Queue::push(int x) {
	if (isFull()) {
		pop();
	}
	if (isEmpty()) {
		front = rear = 0;
	}
	else {
		rear = (rear + 1) % MAX_SIZE;
	}
	A[rear] = x;
//	cout << "Queue: ";
//	for (int i=0; i < rear+1; ++i) {
//		cout << A[i] << " ";
//	}
//	cout << endl;
}

// Removes an element in Queue from front end.
int Queue::pop() {
	int ret = A[front];
	if (isEmpty()) {
		cout << "Error: Queue is Empty\n";
		return 0;
	}
	else if (front == rear) {
		rear = front = -1;
	}
	else {
		rear--;
		for (int i = 0; i < MAX_SIZE - 1; ++i) {
			A[i] = A[i + 1];
		}
	}
//	cout << "Queue: ";
//	for (int i=0; i < rear+1; ++i) {
//		cout << A[i] << " ";
//	}
//	cout << endl;
	return ret;
}
// Returns element at front of queue.
int Queue::Front() {
	if (front == -1) {
		cout << "Error: cannot return front from empty queue\n";
		return -1;
	}
	return A[front];
}
/*
 Printing the elements in queue from front to rear.
 This function is only to test the code.
 This is not a standard function for Queue implementation.
 */
void Queue::Print() {
	// Finding number of elements in queue
	int count = (rear + MAX_SIZE - front) % MAX_SIZE + 1;
	cout << "Queue       : ";
	for (int i = 0; i < count; i++) {
		int index = (front + i) % MAX_SIZE; // Index of element while travesing circularly from front
		cout << A[index] << " ";
	}
	cout << "\n\n";
}
