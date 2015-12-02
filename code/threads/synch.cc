// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue) {
	name = debugName;
	value = initialValue;
	queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore() {
	delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void Semaphore::P() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

	while (value == 0) { 			// semaphore not available
		queue->Append((void *) currentThread);	// so go to sleep
		currentThread->Sleep();
	}
	value--; 					// semaphore available,
	// consume its value

	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void Semaphore::V() {
	Thread *thread;
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	thread = (Thread *) queue->Remove();
	if (thread != NULL)	   // make thread ready, consuming the V immediately
		scheduler->ReadyToRun(thread);
	value++;
	(void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
	name = new char[20];
	sprintf(name, debugName);
	state = AVAILABLE;
	ownerThread = NULL;
	queue = new List;
}
Lock::~Lock() {
	delete queue;
}
bool Lock::isHeldByCurrentThread() {
//	printf("Current thread: %s\n", currentThread->getName());
//	if(ownerThread != NULL) printf("Owner thread: %s\n", ownerThread->getName());
//	else printf("Owner thread: null\n");
	return (currentThread == ownerThread);
}

bool Lock::isBusy() {
	return (state == BUSY);
}

void Lock::Acquire() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
//	printf("%s is trying to acquire lock %s.\n", currentThread->getName(),
//			name);
	if (isHeldByCurrentThread()) {
		DEBUG('t', "\tThread %s already has the lock %s!\n", currentThread->getName(),
				name);
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	if (state == BUSY) {
		queue->Append((void *) currentThread);
		DEBUG('t',"\t%s is busy, owned by thread %s\n", name, ownerThread->getName());
		currentThread->Sleep();
	}
	state = BUSY;
	ownerThread = currentThread;

//	DEBUG('t', "\t%s acquired lock %s.\n", ownerThread->getName(), name);
	(void) interrupt->SetLevel(oldLevel);
}
void Lock::Release() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if (!isHeldByCurrentThread()) {
		printf("\tNon-owner thread %s cannot release lock %s!\n",
				currentThread->getName(), name);
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	else if (!queue->IsEmpty()) {
		DEBUG('t',"\t%s released lock %s. ", ownerThread->getName(), name);
		ownerThread = (Thread *) queue->Remove();
		DEBUG('t',"\tNew owner is %s.\n", ownerThread->getName());
		scheduler->ReadyToRun(ownerThread);
	}
	else {
		state = AVAILABLE;
//		DEBUG('t', "\t%s released lock %s.\n", ownerThread->getName(), name);
		ownerThread = NULL;
	}
//			printf("%s released lock %s.\n", currentThread->getName(), name);
	(void) interrupt->SetLevel(oldLevel);
}

Condition::Condition(char* debugName) {
	name = new char[20];
	sprintf(name, debugName);
	waitingLock = NULL;
	queue = new List;
#ifdef NETWORK
	waitingLockIndex = -1;
	fauxLock = new Lock("Faux Lock");
#endif
}

Condition::~Condition() {
	delete name;
	delete waitingLock;
	delete queue;
#ifdef NETWORK
	delete fauxLock;
#endif
}

bool Condition::isQueueEmpty() {
	return (queue->IsEmpty());
}

void Condition::Wait(Lock* conditionLock) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

	if(!conditionLock->isHeldByCurrentThread()) {
		printf("Trying to call Wait on Lock not owned by %s\n", currentThread->getName());
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}

	if (conditionLock == NULL) {
		// print message
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	if (waitingLock == NULL) {
		// no one waiting
		waitingLock = conditionLock;
	}
	if (waitingLock != conditionLock) {
		// lock is the one being given up
		printf("\tWaiting lock %s is not the same as lock %s.\n",
				waitingLock->getName(), conditionLock->getName());
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	// OK to wait
	queue->Append((void *) currentThread);
	conditionLock->Release();
	currentThread->Sleep();
	conditionLock->Acquire();
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

void Condition::Signal(Lock* conditionLock) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
//	printf("%s is signaling on %s.\n", currentThread->getName(),
//			conditionLock->getName());
	if(!conditionLock->isHeldByCurrentThread()) {
		printf("Trying to call Signal on Lock not owned by %s\n", currentThread->getName());
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	if (queue->IsEmpty()) {
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	if (waitingLock != conditionLock) {
		// print message
		printf("\tWaiting lock %s is not the same as lock %s.\n",
				waitingLock->getName(), conditionLock->getName());
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	Thread* waitingThread = (Thread *) queue->Remove();
	scheduler->ReadyToRun(waitingThread); // put in ready queue
	if (queue->IsEmpty()) {
		waitingLock = NULL;
	}
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

void Condition::Broadcast(Lock* conditionLock) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	if(!conditionLock->isHeldByCurrentThread()) {
		printf("Trying to call Broadcast on Lock not owned by %s", currentThread->getName());
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	if (conditionLock == NULL) {
		// print message
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	if (conditionLock != waitingLock) {
		// print message
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return;
	}
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
	while (!queue->IsEmpty()) {
		Signal(conditionLock);
	}
}

#ifdef NETWORK
int Condition::Wait(int conditionLockIndex) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

	if (conditionLockIndex < 0) {
		// print message
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}
	if (waitingLockIndex < 0) {
		// no one waiting
		waitingLockIndex = conditionLockIndex;
	}
	if (waitingLockIndex != conditionLockIndex) {
		// lock is the one being given up
		printf("\tWaiting lock %i is not the same as lock %i.\n",
				waitingLockIndex, conditionLockIndex);
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}
	// OK to wait
//	queue->Append((void *) currentThread);
//	fauxLock->Release();
	currentThread->Sleep();
//	fauxLock->Acquire();
//	fauxLock->Release();
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
	return 1;
}

int Condition::Signal(int conditionLockIndex) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	if (conditionLockIndex < 0) {
		// print message
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}
	if (waitingLockIndex < 0) {
		// no one waiting
		waitingLockIndex = conditionLockIndex;
	}
	if (waitingLockIndex != conditionLockIndex) {
		// lock is the one being given up
		printf("\tWaiting lock %i is not the same as lock %i.\n",
				waitingLockIndex, conditionLockIndex);
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}
	if (queue->IsEmpty()) {
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}

	Thread* waitingThread = (Thread *) queue->Remove();
	scheduler->ReadyToRun(waitingThread); // put in ready queue
	if (queue->IsEmpty()) {
		waitingLockIndex = -1;
	}
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
	return 1;
}

int Condition::Broadcast(int conditionLockIndex) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	if (conditionLockIndex < 0) {
		// print message
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}
	if (waitingLockIndex < 0) {
		// no one waiting
		waitingLockIndex = conditionLockIndex;
	}
	if (waitingLockIndex != conditionLockIndex) {
		// lock is the one being given up
		printf("\tWaiting lock %i is not the same as lock %i.\n",
				waitingLockIndex, conditionLockIndex);
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		return -1;
	}
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
	while (!queue->IsEmpty()) {
		Signal(conditionLockIndex);
	}
	return 1;
}

#endif

Monitor::Monitor(char* debugName, int maxSize) {
	name = new char[20];
	sprintf(name, debugName);
//	val = new int[size];
	val = new vector<int>(maxSize, 0);
	size = maxSize;
}

Monitor::~Monitor() {
	delete name;
	delete val;
}

int Monitor::getVal(int position) {
	if (position < 0 || position >= size) {
		// printf("Error: invalid monitor value position.\n");
		return -1;
	}
	return (*val)[position];
}

void Monitor::setVal(int position, int value) {
	if (position < 0 || position >= size) {
		// printf("Error: invalid monitor value position.\n");
		return;
	}
	(*val)[position] = value;
}
