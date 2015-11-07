// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <stdio.h>
#include <iostream>

using namespace std;

void Create_Syscall(unsigned int vaddr, int len);
int Open_Syscall(unsigned int vaddr, int len);
void Write_Syscall(unsigned int vaddr, int len, int id);
int Read_Syscall(unsigned int vaddr, int len, int id);
void Close_Syscall(int fd);
void Exit_Syscall(int status);
void exec_thread(int vaddr);
void Exec_Syscall(int vaddr, int len);
void kernel_thread(int vaddr);
void Fork_Syscall(int vaddr, int len);
void Yield_Syscall();
void Acquire_Syscall(int index);
void Release_Syscall(int index);
void Wait_Syscall(int conditionIndex, int lockIndex);
void Signal_Syscall(int conditionIndex, int lockIndex);
void Broadcast_Syscall(int conditionIndex, int lockIndex);
int CreateLock_Syscall(int vaddr, int len);
void DestroyLock_Syscall(int index);
int CreateCondition_Syscall(int vaddr, int len);
void DestroyCondition_Syscall(int index);
int Rand_Syscall();
void PrintInt_Syscall(int num);
void Send_Syscall(PacketHeader pktHdr, PacketHeader mailHdr, char *data);
void Receive_Syscall(int box, PacketHeader *pktHdr, PacketHeader *mailHdr,
		char *data);

int copyin(unsigned int vaddr, int len, char *buf) {
	// Copy len bytes from the current thread's virtual address vaddr.
	// Return the number of bytes so read, or -1 if an error occors.
	// Errors can generally mean a bad virtual address was passed in.
	bool result;
	int n = 0;			// The number of bytes copied in
	int *paddr = new int;

	while (n >= 0 && n < len) {
		result = machine->ReadMem(vaddr, 1, paddr);
		while (!result) // FALL 09 CHANGES
		{
			result = machine->ReadMem(vaddr, 1, paddr); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
		}

		buf[n++] = *paddr;

		if (!result) {
			//translation failed
			return -1;
		}

		vaddr++;
	}

	delete paddr;
	return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
	// Copy len bytes to the current thread's virtual address vaddr.
	// Return the number of bytes so written, or -1 if an error
	// occors.  Errors can generally mean a bad virtual address was
	// passed in.
	bool result;
	int n = 0;			// The number of bytes copied in

	while (n >= 0 && n < len) {
		// Note that we check every byte's address
		result = machine->WriteMem(vaddr, 1, (int) (buf[n++]));

		if (!result) {
			//translation failed
			return -1;
		}

		vaddr++;
	}

	return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
	// Create the file with the name in the user buffer pointed to by
	// vaddr.  The file name is at most MAXFILENAME chars long.  No
	// way to return errors, though...
	char *buf = new char[len + 1];	// Kernel buffer to put the name in

	if (!buf)
		return;

	if (copyin(vaddr, len, buf) == -1) {
		printf("%s", "Bad pointer passed to Create\n");
		delete buf;
		return;
	}

	buf[len] = '\0';

	fileSystem->Create(buf, 0);
	delete[] buf;
	return;
}

int Open_Syscall(unsigned int vaddr, int len) {
	// Open the file with the name in the user buffer pointed to by
	// vaddr.  The file name is at most MAXFILENAME chars long.  If
	// the file is opened successfully, it is put in the address
	// space's file table and an id returned that can find the file
	// later.  If there are any errors, -1 is returned.
	char *buf = new char[len + 1];	// Kernel buffer to put the name in
	OpenFile *f;			// The new open file
	int id;				// The openfile id

	if (!buf) {
		printf("%s", "Can't allocate kernel buffer in Open\n");
		return -1;
	}

	if (copyin(vaddr, len, buf) == -1) {
		printf("%s", "Bad pointer passed to Open\n");
		delete[] buf;
		return -1;
	}

	buf[len] = '\0';

	f = fileSystem->Open(buf);
	delete[] buf;

	if (f) {
		if ((id = currentThread->space->fileTable.Put(f)) == -1)
			delete f;
		return id;
	}
	else
		return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
	// Write the buffer to the given disk file.  If ConsoleOutput is
	// the fileID, data goes to the synchronized console instead.  If
	// a Write arrives for the synchronized Console, and no such
	// console exists, create one. For disk files, the file is looked
	// up in the current address space's open file table and used as
	// the target of the write.

	char *buf;		// Kernel buffer for output
	OpenFile *f;	// Open file for output

	if (id == ConsoleInput)
		return;

	if (!(buf = new char[len])) {
		printf("%s", "Error allocating kernel buffer for write!\n");
		return;
	}
	else {
		if (copyin(vaddr, len, buf) == -1) {
			printf("%s", "Bad pointer passed to to write: data not written\n");
			delete[] buf;
			return;
		}
	}

	if (id == ConsoleOutput) {
		for (int ii = 0; ii < len; ii++) {
			printf("%c", buf[ii]);
		}

	}
	else {
		if ((f = (OpenFile *) currentThread->space->fileTable.Get(id))) {
			f->Write(buf, len);
		}
		else {
			printf("%s", "Bad OpenFileId passed to Write\n");
			len = -1;
		}
	}

	delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
	// Write the buffer to the given disk file.  If ConsoleOutput is
	// the fileID, data goes to the synchronized console instead.  If
	// a Write arrives for the synchronized Console, and no such
	// console exists, create one.    We reuse len as the number of bytes
	// read, which is an unnessecary savings of space.
	char *buf;		// Kernel buffer for input
	OpenFile *f;	// Open file for output

	if (id == ConsoleOutput)
		return -1;

	if (!(buf = new char[len])) {
		printf("%s", "Error allocating kernel buffer in Read\n");
		return -1;
	}

	if (id == ConsoleInput) {
		//Reading from the keyboard
		scanf("%s", buf);

		if (copyout(vaddr, len, buf) == -1) {
			printf("%s", "Bad pointer passed to Read: data not copied\n");
		}
	}
	else {
		if ((f = (OpenFile *) currentThread->space->fileTable.Get(id))) {
			len = f->Read(buf, len);
			if (len > 0) {
				//Read something from the file. Put into user's address space
				if (copyout(vaddr, len, buf) == -1) {
					printf("%s",
							"Bad pointer passed to Read: data not copied\n");
				}
			}
		}
		else {
			printf("%s", "Bad OpenFileId passed to Read\n");
			len = -1;
		}
	}

	delete[] buf;
	return len;
}

void Close_Syscall(int fd) {
	// Close the file associated with id fd.  No error reporting.
	OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

	if (f) {
		delete f;
	}
	else {
		printf("%s", "Tried to close an unopen file\n");
	}
}

void Exit_Syscall(int status) {
	// All of the parent/child stuff from the documentation is only relevant for Join and we shouldn't have to worry about that at all

	/*3 Exit Cases
	 (1) a thread calls Exit - not the last executing thread in the process
	 Reclaim 8 pages of stack
	 (2) last executing thread in last process (i.e. ready queue is empty)
	 No need to reclaim anything
	 Stop Nachos
	 (3) Last executing thread in a process - not last process
	 Reclaim all unreclaimed memory
	 Locks/CVs (match AddrSpace* w/ Process Table)*/
	currentThread->Finish();
	processLock.Acquire();

	//find the current thread we are in
	process * myProcess = processTable[currentThread->space->processIndex];

	if (myProcess->numThreadsRunning > 1) { //we not the last thread in the process
		//reclaim 8 pages of the stack

		bitmapLock.Acquire();

		//find the beginning of this thread's stack
		int pageTableIndex = myProcess->threadStacks[currentThread->threadIndex];
		for (int i = pageTableIndex; i < pageTableIndex + 8; i++) {
			bitmap.Clear(currentThread->space->getPageTable()[i].physicalPage);
		}

		myProcess->numThreadsRunning -= 1;

		bitmapLock.Release();
		processLock.Release();
		currentThread->Finish();
	}
	else {
		//we are the last thread in the process
		if (activeProcesses == 1) { //we are the last process in nachos
			processLock.Release();
			activeProcesses--;
			interrupt->Halt();
		}
		else {
			bitmapLock.Acquire();
			activeProcesses--;

			//reclaim the entire page table of the process
			for (unsigned int i = 0; i < currentThread->space->numPages; i++) {
				bitmap.Clear(
						currentThread->space->getPageTable()[i].physicalPage);
			}

			bitmapLock.Release();
			processLock.Release();
			currentThread->Finish();
		}
	}
}

/* Helper function for Exec */
void exec_thread(int vaddr) {
	// Initialize the register by using currentThread->space
	currentThread->space->InitRegisters();
	// Call Restore State through currentThread->space
//	currentThread->space->RestoreState();
	// Call machine->Run()
	machine->Run();
}

void Exec_Syscall(int vaddr, int len) {

	processLock.Acquire();
	if (processIndex >= NUM_PROCESSES) {
		printf("Too many processes.\n");
		processLock.Release();
		return;
	}

	// Convert it to the physical address and read the contents from there, which will give you the name of the process to be executed
	// Now Open that file using filesystem->Open
	char *buf = new char[len + 1];  // Kernel buffer to put the name in
	OpenFile *f;      // The new open file

	if (!buf) {
		printf("%s", "Can't allocate kernel buffer in Open\n");
		return;
	}

	if (copyin(vaddr, len, buf) == -1) {
		printf("%s", "Bad pointer passed to Open\n");
		delete[] buf;
		return;
	}

	buf[len] = '\0';

	// Store its openfile pointer
	f = fileSystem->Open(buf);
	if (f == NULL) {
		printf("Cannot execute -- no such executable \"%s\".\n", buf);
		return;
	}
	delete[] buf;

	// Create new addrespace for this executable file and update process table
	AddrSpace* a = new AddrSpace(f);
	a->processIndex = processIndex;
	a->addStack();
	delete f;

	process *p = new process;
	p->threadStacks = new int[50]; // max number of threads is 50
	p->numThreadsTotal = 1;
	p->numThreadsRunning = 1; // when does this get incremented???
	p->threadStacks[0] = a->numPages; //first thread's stack starts at 0?
	processTable[processIndex] = p;

	processIndex++;
	activeProcesses++;

	// Create a new thread
	Thread* t = new Thread("exec_thread", 0);

	processLock.Release();

	// Allocate the space created to this thread's space
	t->space = a;

	// Fork the new thread. I call it exec_thread
//	printf("\tForking in exec.\n");
	t->Fork((VoidFunctionPtr) exec_thread, vaddr);
}

/* Helper function for Fork */
void kernel_thread(int vaddr) {
	DEBUG('t', "In kernel thread for thread index %i!!!\n",
			currentThread->threadIndex);
	// Write to the register PCReg the virtual address
	machine->WriteRegister(PCReg, vaddr);
	// Write virtualaddress + 4 in NextPCReg
	machine->WriteRegister(NextPCReg, vaddr + 4);
	// Write to the stack register, the starting position of the stack (addr of the first page) for this thread

	machine->WriteRegister(StackReg,
			processTable[currentThread->space->processIndex]->threadStacks[currentThread->threadIndex]
					* PageSize - 16);
	// Call Restorestate function inorder to prevent information loss while context switching
	currentThread->space->RestoreState();
	// Call machine->Run()
	machine->Run();
}

void Fork_Syscall(int vaddr, int len) {
	DEBUG('t', "Entering Fork_Syscall\n");

	// Get the current process from the Process Table so we can use it for everything else
	processLock.Acquire();
	process* p = processTable[currentThread->space->processIndex];
	// Create a New thread. This would be a kernel thread
	char * name = new char[20];
	sprintf(name, "kernel_thread %i", p->numThreadsTotal);
	Thread* t = new Thread(name, p->numThreadsTotal);
	// Update the Process Table for Multiprogramming part
	int oldPageTableIndex = p->threadStacks[currentThread->threadIndex];
	TranslationEntry* oldPageTable = currentThread->space->getPageTable();
	currentThread->space->expandTable();
	/*updates the MACHINE'S page table (the registers currently in CPU basically).
	 setting the page table makes it correct for all FUTURE times the thread is swapped
	 in the CPU but we still need to change the current registers to use the new page table before
	 we delete the old one and garbage starts getting written to it*/

	// delete old page table?
	p->threadStacks[t->threadIndex] = currentThread->space->numPages;
//	printf("Thread stack %i set to %i\n",t->threadIndex, currentThread->space->numPages);
	p->numThreadsTotal++;
	p->numThreadsRunning++;
	// Allocate the addrespace to the thread being forked which is essentially current thread's addresspsace
	// because threads share the process addressspace
	t->space = currentThread->space;

//	delete[] oldPageTable;
//	oldPageTable = NULL;
//	currentThread->space->RestoreState();
	processLock.Release();

	t->Fork(kernel_thread, vaddr);
}

void Yield_Syscall() {
	// Just call currentThread->Yield()
	currentThread->Yield();
}

void Acquire_Syscall(int index) {
	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[index].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread\n.",
				kernelLockList[index].lock->getName());
		kernelLockLock.Release();
		return;
	}
	kernelLockLock.Release();
	DEBUG('t', "Acquiring lock %s.\n", kernelLockList[index].lock->getName());
	kernelLockList[index].lock->Acquire();
}

void Release_Syscall(int index) {
	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[index].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[index].lock->getName());
		kernelLockLock.Release();
		return;
	}
	kernelLockLock.Release();
	DEBUG('t', "Releasing lock %s.\n", kernelLockList[index].lock->getName());
	kernelLockList[index].lock->Release();

	if (kernelLockList[index].isToBeDeleted) {
		DestroyLock_Syscall(index);
	}
}

void Wait_Syscall(int conditionIndex, int lockIndex) {
	kernelConditionLock.Acquire();
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
		kernelConditionLock.Release();
		return;
	}

	kernelLockLock.Acquire();
	// Check lock
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelConditionLock.Release();
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		kernelConditionLock.Release();
		kernelLockLock.Release();
		return;
	}

	DEBUG('t', "Condition [%s] waiting on lock [%s].\n",
			kernelConditionList[conditionIndex].condition->getName(),
			kernelLockList[lockIndex].lock->getName());
	kernelLockLock.Release();
	kernelConditionLock.Release();
	kernelConditionList[conditionIndex].condition->Wait(
			kernelLockList[lockIndex].lock);
}

void Signal_Syscall(int conditionIndex, int lockIndex) {
	kernelConditionLock.Acquire();
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
		kernelConditionLock.Release();
		return;
	}

	// Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	DEBUG('t', "Condition [%s] signaling on lock [%s].\n",
			kernelConditionList[conditionIndex].condition->getName(),
			kernelLockList[lockIndex].lock->getName());
	kernelLockLock.Release();
	kernelConditionLock.Release();
	kernelConditionList[conditionIndex].condition->Signal(
			kernelLockList[lockIndex].lock);

	if (kernelConditionList[conditionIndex].isToBeDeleted) {
		DestroyCondition_Syscall(conditionIndex);
	}
}

void Broadcast_Syscall(int conditionIndex, int lockIndex) {
	kernelConditionLock.Acquire();
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
		kernelConditionLock.Release();
		return;
	}

	// Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to broadcast.\n");
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return;
	}

	DEBUG('t', "Condition [%s] broadcasting on lock [%s].\n",
			kernelConditionList[conditionIndex].condition->getName(),
			kernelLockList[lockIndex].lock->getName());
	kernelLockLock.Release();
	kernelConditionLock.Release();
	kernelConditionList[conditionIndex].condition->Broadcast(
			kernelLockList[lockIndex].lock);

	if (kernelConditionList[conditionIndex].isToBeDeleted) {
		DestroyCondition_Syscall(conditionIndex);
	}
}

int CreateLock_Syscall(int vaddr, int len) {
	// Create the lock with the name in the user buffer pointed to by
	// vaddr.  The lock name is at most MAXFILENAME chars long.  No
	// way to return errors, though...
	char *buf = new char[len + 1];  // Kernel buffer to put the name in

	if (!buf)
		return -1;
	if (copyin(vaddr, len, buf) == -1) {
		printf("%s", "Bad pointer passed to CreateLock\n");
		delete buf;
		return -1;
	}

	buf[len] = '\0';

	// Create the new lock...
	kernelLockLock.Acquire();
	int createdLockIndex = kernelLockIndex;
	kernelLockList[kernelLockIndex].lock = new Lock(buf);
	kernelLockList[kernelLockIndex].addrsp = currentThread->space; // #userprog
	kernelLockList[kernelLockIndex].isToBeDeleted = false;
	// the next new lock's index
	kernelLockIndex++;
	kernelLockLock.Release();

	delete[] buf;
	DEBUG('t', "Creating lock %s, index %i.\n", buf, createdLockIndex);
	return createdLockIndex;
}

void DestroyLock_Syscall(int index) {
	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to destroy.\n");
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		kernelLockLock.Release();
		return;
	}

	if (kernelLockList[index].lock->isBusy()) {
		DEBUG('t', "Lock %s is still busy. Won't destroy.\n",
				kernelLockList[index].lock->getName());
		kernelLockList[kernelLockIndex].isToBeDeleted = true;
		kernelLockLock.Release();
		return;
	}

	if (!kernelLockList[index].lock->isBusy()
			|| kernelLockList[kernelLockIndex].isToBeDeleted) {
		DEBUG('t', "Lock %s will be destroyed.\n",
				kernelLockList[index].lock->getName());
		kernelLockList[kernelLockIndex].isToBeDeleted = true;
		kernelLockList[kernelLockIndex].lock = NULL;
	}
	kernelLockLock.Release();
}

int CreateCondition_Syscall(int vaddr, int len) {
	// Create the condition variable with the name in the user buffer pointed to by
	// vaddr.  The condition variable name is at most MAXFILENAME chars long.  No
	// way to return errors, though...
	char *buf = new char[len + 1];  // Kernel buffer to put the name in

	if (!buf)
		return -1;
	if (copyin(vaddr, len, buf) == -1) {
		printf("%s", "Bad pointer passed to CreatCondition\n");
		delete buf;
		return -1;
	}

	buf[len] = '\0';

	// Create the new condition variable...
	kernelConditionLock.Acquire();
	int createdConditionIndex = kernelConditionIndex;
	kernelConditionList[kernelConditionIndex].condition = new Condition(buf);
	kernelConditionList[kernelConditionIndex].addrsp = currentThread->space;
	kernelConditionList[kernelConditionIndex].isToBeDeleted = false;
	kernelConditionIndex++;
	kernelConditionLock.Release();

	delete[] buf;
	DEBUG('t', "Creating condition %s, index %i.\n", buf,
			createdConditionIndex);
	return createdConditionIndex;
}

void DestroyCondition_Syscall(int index) {
	kernelConditionLock.Acquire();
	if (index < 0 || index >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[index].condition == NULL) {
		printf("No condition at index %i.\n", index);
		kernelConditionLock.Release();
		return;
	}

	if (!kernelConditionList[index].condition->isQueueEmpty()) {
		DEBUG('t', "Threads are still using condition %s. Won't destroy.\n",
				kernelConditionList[index].condition->getName());
		kernelConditionList[kernelConditionIndex].isToBeDeleted = true;
		kernelConditionLock.Release();
		return;
	}

	if (kernelConditionList[index].condition->isQueueEmpty()
			|| kernelConditionList[kernelConditionIndex].isToBeDeleted) {
		DEBUG('t', "Condition %s will be destroyed.\n",
				kernelConditionList[index].condition->getName());
		kernelConditionList[kernelConditionIndex].isToBeDeleted = true;
		kernelConditionList[kernelConditionIndex].condition = NULL;
	}
	kernelConditionLock.Release();
}

int Rand_Syscall() {
	return Random();
}

void PrintInt_Syscall(int num) {
	printf("%i", num);
}

void Send_Syscall(PacketHeader pktHdr, PacketHeader mailHdr, char *data) {

}

void Receive_Syscall(int box, PacketHeader *pktHdr, PacketHeader *mailHdr,
		char *data) {

	int type = 0; // TODO: use stringstream to get this from data

	switch (type) {
	default:
		DEBUG('a', "Unknown remote procedure call!\n");
	case RPC_CreateLock:
		DEBUG('a', "CreateLock remote procedure call.\n");
		// TODO: networking stuff
		break;
	case RPC_DestroyLock:
		DEBUG('a', "DestroyLock remote procedure call.\n");
		// TODO: networking stuff
		break;
	case RPC_CreateCondition:
		DEBUG('a', "CreateCondition remote procedure call.\n");
		// TODO: networking stuff
		break;
	case RPC_DestroyCondition:
		DEBUG('a', "DestroyCondition remote procedure call.\n");
		// TODO: networking stuff
		break;
		// TODO: monitor variable RPCs
	}
}

int handleMemoryFull() {
	//--- Find PPN to evict --//
	// if random
	int evictPPN = -1;
	int swapWriteLoc = -1;
	if (!isFIFO) {
		evictPPN = rand() % NumPhysPages;
		// if dirty write to swap file
	}
	// if FIFO
	else if (isFIFO) {
		if (pageQueue.isEmpty()) {
			// spmething
		}
		ASSERT(!pageQueue.isEmpty())
		// page to be evicted
		int vpn = pageQueue.pop();
		// find the PPN where the VP is loaded
		for (int i = 0; i < NumPhysPages; ++i) {
			if (ipt[i].virtualPage == vpn) {
				evictPPN = i;
				break;
			}
		}
	}
	// update TLB
	for (int i = 0; i < TLBSize; ++i) {
		if (evictPPN == machine->tlb[i].physicalPage) {
			ipt[evictPPN].dirty = machine->tlb[evictPPN].dirty;
			machine->tlb[evictPPN].valid = FALSE;
		}
	}

	//-- Write to Swap --//
	swapLock.Acquire();
	if (swapFile == NULL) {
		swapFile = fileSystem->Open(swapFileName);
	}
	swapLock.Release();
	// change to some sort of handling
	ASSERT(swapFile != NULL);

	// page replacement
	iptLock.Acquire();
	swapLock.Acquire();
	if (ipt[evictPPN].dirty) {
		// update page table
		DEBUG('v', "\tDirty and writing to SWAP\n");
		int vpn = ipt[evictPPN].virtualPage;
//		currentThread->space->getPageTable()[vpn].dirty = TRUE;
		swapWriteLoc = swapFileBM.Find();
		// handling
		if (swapWriteLoc < 0) {
			DEBUG('v', "\tSWAP OUT OF SPACE!!!\n");
		}
		else {
			// WHAT ARE THE ARGS?
			swapFile->WriteAt(&machine->mainMemory[evictPPN * PageSize],
					PageSize, swapWriteLoc * PageSize);
		}

//		int vpn = ipt[evictPPN].virtualPage;
		ASSERT(vpn == ipt[evictPPN].virtualPage);
		currentThread->space->getPageTable()[vpn].physicalPage = -1;
		currentThread->space->getPageTable()[vpn].diskLocation =
				PageTableEntry::SWAP;
		currentThread->space->getPageTable()[vpn].byteOffset = swapWriteLoc;
	}
	swapLock.Release();
	iptLock.Release();
	// return ppn for handleIPTMiss to populate IPT
	return evictPPN;
}

int handleIPTMiss(int vpn) {
	//-- We will check for location of the page --//
	int ppn = bitmap.Find();
	if (ppn == -1) {
		DEBUG('v', "\tMemory is full\n");
		ppn = handleMemoryFull();
		// swap file shit
	}

	//--- Look in page table for VPN --//
	pageTableLock.Acquire();
	PageTableEntry * pte = &currentThread->space->getPageTable()[vpn];
	//--- Find location of VP, swap or exe, load in memory, update page tables --//
	//--- EXECUTABLE --//
	if (pte->diskLocation == PageTableEntry::EXECUTABLE) {
		DEBUG('v', "\tVP is in EXE\n");
		// load into memory
		currentThread->space->myExecutable->ReadAt(
				&(machine->mainMemory[PageSize * ppn]),
				PageSize, pte->byteOffset);
		// not dirty because freshly loaded
		currentThread->space->getPageTable()[vpn].dirty = FALSE;
	}
	//-- SWAP --//
	else if (pte->diskLocation == PageTableEntry::SWAP) {
		DEBUG('v', "\tVP is in SWAP\n");
	}

	pte->physicalPage = ppn;
	pte->valid = TRUE;

	//-- We got some ppn, now put into IPT --//
	iptLock.Acquire();
	ipt[ppn].virtualPage = vpn;
	ipt[ppn].physicalPage = ppn;
	ipt[ppn].valid = TRUE;
	ipt[ppn].use = pte->use;
	ipt[ppn].dirty = pte->dirty;
	ipt[ppn].readOnly = pte->readOnly;
	ipt[ppn].space = currentThread->space; // space pointers
	ipt[ppn].byteOffset = pte->byteOffset;
	ipt[ppn].diskLocation = pte->diskLocation;
	iptLock.Release();
	pageTableLock.Release();

	return ppn;
}

void handlePageFault() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	int vpn = machine->ReadRegister(BadVAddrReg) / PageSize;
	DEBUG('v', "\tMissed vpn %i\n", vpn);
	int ppn = -1;

	//--- Look in IPT for needed VPN ---//
	iptLock.Acquire();
	for (int i = 0; i < NumPhysPages; ++i) {
		if (vpn == ipt[i].virtualPage && ipt[i].valid && currentThread->space == ipt[i].space) {
			ppn = i;
			DEBUG('v', "\tFound VPN %i at PPN %i\n", ipt[i].virtualPage, ppn);
			break;
		}
	}
	iptLock.Release();

	//--- Not found in IPT (IPT miss) --//
	if (ppn < 0 || ppn >= NumPhysPages) {
		DEBUG('v', "\tIPT miss\n");
		ppn = handleIPTMiss(vpn);
	}
	DEBUG('v', "\tTried IPT, PPN is %i\n", ppn);

	if (ppn < 0) {
		DEBUG('v', "\tVirtual page number %i does not exist in IPT.\n", vpn);
	}
	//--- Continue after IPT miss or not IPT miss --//
	//--- Update TLB, compute PA --//
	else {
		// Must update IPT if tlb entry not dirty and is valid
		if (machine->tlb[currentTLBEntry].dirty
				&& machine->tlb[currentTLBEntry].valid) {
			DEBUG('v', "\tUpdating IPT and PT because TLB is dirty and valid\n");
//			int ppn = machine->tlb[currentTLBEntry].physicalPage;
			ASSERT(ppn == machine->tlb[currentTLBEntry].physicalPage);
			ipt[ppn].dirty = TRUE;
//			ipt[ppn]
			ASSERT(vpn == machine->tlb[currentTLBEntry].virtualPage);
			currentThread->space->getPageTable()[vpn].dirty = TRUE;
		}

		iptLock.Acquire();
		IPTEntry entry = ipt[ppn];
		DEBUG('v', "\tPPN is %i\n", ppn);
		DEBUG('v', "\tPutting at currentTLBEntry %i\n", currentTLBEntry);
		machine->tlb[currentTLBEntry].physicalPage = entry.physicalPage;
		machine->tlb[currentTLBEntry].virtualPage = entry.virtualPage;
		machine->tlb[currentTLBEntry].valid = true;
		machine->tlb[currentTLBEntry].dirty = false;
		machine->tlb[currentTLBEntry].use = false;
		currentTLBEntry = (currentTLBEntry + 1) % TLBSize;
		iptLock.Release();
	}
	(void) interrupt->SetLevel(oldLevel);
}

void ExceptionHandler(ExceptionType which) {
	int type = machine->ReadRegister(2); // Which syscall?
	int rv = 0; 	// the return value from a syscall

	if (which == SyscallException) {
		switch (type) {
		default:
			DEBUG('a', "Unknown syscall - shutting down.\n");
		case SC_Halt:
			DEBUG('a', "Shutdown, initiated by user program.\n");
			interrupt->Halt();
			break;
		case SC_Create:
			DEBUG('a', "Create syscall.\n");
			Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_Open:
			DEBUG('a', "Open syscall.\n");
			rv = Open_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
			break;
		case SC_Write:
			DEBUG('a', "Write syscall.\n");
			Write_Syscall(machine->ReadRegister(4), machine->ReadRegister(5),
					machine->ReadRegister(6));
			break;
		case SC_Read:
			DEBUG('a', "Read syscall.\n");
			rv = Read_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5), machine->ReadRegister(6));
			break;
		case SC_Close:
			DEBUG('a', "Close syscall.\n");
			Close_Syscall(machine->ReadRegister(4));
			break;
		case SC_Fork:
			DEBUG('a', "Fork syscall.\n");
			Fork_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_Yield:
			DEBUG('a', "Yield syscall.\n");
			Yield_Syscall();
			break;
		case SC_Acquire:
			DEBUG('a', "Acquire syscall.\n");
			Acquire_Syscall(machine->ReadRegister(4));
			break;
		case SC_Release:
			DEBUG('a', "Release syscall.\n");
			Release_Syscall(machine->ReadRegister(4));
			break;
		case SC_Wait:
			DEBUG('a', "Wait syscall.\n");
			Wait_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_Signal:
			DEBUG('a', "Signal syscall.\n");
			Signal_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_Broadcast:
			DEBUG('a', "Broadcast syscall.\n");
			Broadcast_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
			break;
		case SC_CreateLock:
			DEBUG('a', "CreateLock syscall.\n");
			// Send_Syscall(/* TODO: fill this in */);
			// rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_DestroyLock:
			DEBUG('a', "DestroyLock syscall.\n");
			// Send_Syscall(/* TODO: fill this in */);
			// DestroyLock_Syscall(machine->ReadRegister(4));
			break;
		case SC_CreateCondition:
			DEBUG('a', "CreateCondition syscall.\n");
			// Send_Syscall(/* TODO: fill this in */);
			// rv = CreateCondition_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_DestroyCondition:
			DEBUG('a', "DestroyCondition syscall.\n");
			// Send_Syscall(/* TODO: fill this in */);
			// DestroyCondition_Syscall(machine->ReadRegister(4));
			break;
		case SC_Exec:
			DEBUG('a', "Exec syscall.\n");
			Exec_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_Exit:
			DEBUG('a', "Exit syscall.\n");
			Exit_Syscall(machine->ReadRegister(4));
			break;
		case SC_Rand:
			DEBUG('a', "Rand syscall.\n");
			rv = Rand_Syscall();
			break;
		case SC_PrintInt:
			DEBUG('a', "PrintInt syscall.\n");
			PrintInt_Syscall(machine->ReadRegister(4));
			break;
		case SC_Send:
			DEBUG('a', "Send syscall.\n");
			// Send_Syscall(/* TODO: fill this in */);
			break;
		case SC_Receive:
			DEBUG('a', "Receive syscall.\n");
			// Receive_Syscall(/* TODO: fill this in */);
			break;

		}

		// Put in the return value and increment the PC
		machine->WriteRegister(2, rv);
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(PCReg) + 4);
		return;
	}
	else if (which == PageFaultException) {
		DEBUG('v', "Page Fault Exception\n");
		handlePageFault();
//		interrupt->Halt();
	}
	else {
		cout << "Unexpected user mode exception - which:" << which << "  type:"
				<< type << endl;
		interrupt->Halt();
	}
}

