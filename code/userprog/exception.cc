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
#include <string>
#include <sstream>  
#include "post.h"

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
//	currentThread->Finish();
	cout << "Exit result: " << status << endl;
	processLock.Acquire();

	//find the current thread we are in
	process * myProcess = processTable[currentThread->space->processIndex];
	int pp = 0;
	if (myProcess->numThreadsRunning > 1) { //we not the last thread in the process
		//reclaim 8 pages of the stack
		DEBUG('s', "I'm not the last thread, [%s], index %i\n", currentThread->getName(), currentThread->threadIndex);
		bitmapLock.Acquire();

		//find the beginning of this thread's stack
		int pageTableIndex = myProcess->threadStacks[currentThread->threadIndex];
		for (int i = pageTableIndex; i < pageTableIndex + 8; i++) {
			pp = currentThread->space->getPageTable()[i].physicalPage;
			DEBUG('s', "Examining PPN %i\n", pp);

			// must mark tlb and ipt as invalid
			if (ipt[pp].space == currentThread->space) {
				ipt[pp].valid = FALSE;
				currentThread->space->getPageTable()[i].valid = FALSE;
				currentThread->space->getPageTable()[i].physicalPage = -1;
				if(pp >= 0) bitmap.Clear(pp);
				DEBUG('s', "\tMarking PPN %i, VPN %i as invalid and clearing memory.\n", pp, ipt[pp].virtualPage);
			}
			for (int j=0; j < TLBSize; ++j) {
				if (machine->tlb[j].physicalPage == pp) {
					machine->tlb[j].valid = FALSE;
					DEBUG('s', "\tMarking TLB entry PPN %i, VPN %i as invalid.\n", pp, machine->tlb[j].virtualPage);
				}
			}
		}

		myProcess->numThreadsRunning -= 1;

		bitmapLock.Release();
		processLock.Release();
		currentThread->Finish();
	}
	else {
		DEBUG('s', "I [%s] am the last thread\n", currentThread->getName());
		//we are the last thread in the process
		if (activeProcesses == 1) { //we are the last process in nachos
			DEBUG('s', "I'm also the last process\n");
			processLock.Release();
			activeProcesses--;
			interrupt->Halt();
		}
		else {
			bitmapLock.Acquire();
			activeProcesses--;

			DEBUG('s', "I'm not the last process\n");
			//reclaim the entire page table of the process
			for (unsigned int i = 0; i < currentThread->space->numPages; i++) {
				pp = currentThread->space->getPageTable()[i].physicalPage;
				DEBUG('s', "Examining PPN %i\n", pp);

				// must mark tlb and ipt as invalid
				if (ipt[pp].space == currentThread->space) {
					ipt[pp].valid = FALSE;
					currentThread->space->getPageTable()[i].valid = FALSE;
					currentThread->space->getPageTable()[i].physicalPage = -1;
					if(pp >= 0) bitmap.Clear(pp);
					DEBUG('s', "\tMarking PPN %i, VPN %i as invalid and clearing memory.\n", pp, ipt[pp].virtualPage);
				}
				for (int j=0; j < TLBSize; ++j) {
					if (machine->tlb[j].physicalPage == pp) {
						machine->tlb[j].valid = FALSE;
						DEBUG('s', "\tMarking TLB entry PPN %i, VPN %i as invalid.\n", pp, machine->tlb[j].virtualPage);
					}
				}
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
	currentThread->space->RestoreState();
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
//	a->addStack();

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
	DEBUG('s', "Entering Fork_Syscall\n");

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
	DEBUG('s', "Thread ID %i for thread %s\n", t->threadIndex, t->getName());
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

	#ifdef NETWORK
		PacketHeader outPktHdr;
		MailHeader outMailHdr;

		outPktHdr.from = machineNum;
		outPktHdr.to = 0;

		outMailHdr.from = 1;
		outMailHdr.to = 0;

		std::stringstream ss;
		ss << index;
		std::string indexStr = ss.str();
		std::string acquireLock = "3 ";

		std::string message = acquireLock + indexStr;
		outMailHdr.length = strlen(message.c_str()) + 1;
		cout << "Acquire Lock: Sending message: " << message << endl;
		bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

		 if ( !success ) {
      		printf("ACQUIRE LOCK: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      		interrupt->Halt();
    	} 

    	PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

		//imo we should have a receive here just to make sure the action finishes on the server b4 returning
    	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

    	return;
	#endif

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

	#ifdef NETWORK
		PacketHeader outPktHdr;
		MailHeader outMailHdr;

		outPktHdr.from = machineNum;
		outPktHdr.to = 0;

		outMailHdr.from = 1;
		outMailHdr.to = 0;

		std::stringstream ss;
		ss << index;
		std::string indexStr = ss.str();
		std::string releaseLock = "4 ";

		std::string message = releaseLock + indexStr;
		outMailHdr.length = strlen(message.c_str()) + 1;
		cout << "Release Lock: Sending message: " << message << endl;
		bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

		 if ( !success ) {
      		printf("RELEASE LOCK: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      		interrupt->Halt();
    	} 

    	PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

		//imo we should have a receive here just to make sure the action finishes on the server b4 returning
    	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

    	return;
	#endif

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

#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << conditionIndex << " " << lockIndex;
	std::string indexStr = ss.str();
	std::string waitCondition = "7 ";

	std::string message = waitCondition + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Wait Condition: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("WAIT CONDITION: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return;
#endif
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
#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << conditionIndex << " " << lockIndex;
	std::string indexStr = ss.str();
	std::string signalCondition = "8 ";

	std::string message = signalCondition + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Signal Conditon: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("SIGNAL CONDITION: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return;
#endif
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
#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << conditionIndex << " " << lockIndex;
	std::string indexStr = ss.str();
	std::string broadcastCondition = "9 ";

	std::string message = broadcastCondition + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Broadcast Condition: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("BROADCAST CONDTION: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return;
#endif
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

	#ifdef NETWORK
		PacketHeader outPktHdr;
		MailHeader outMailHdr;

		outPktHdr.from = machineNum;
		outPktHdr.to = 0;

		outMailHdr.from = 1;
		outMailHdr.to = 0;

		std::stringstream ss;
		ss << buf;
		std::string nameStr = ss.str();
		std::string createLock = "1 ";

		std::string message = createLock + nameStr;
		outMailHdr.length = strlen(message.c_str()) + 1;
		cout << "Create Lock: Sending message: " << message << endl;
		bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

		 if ( !success ) {
      		printf("CREATE LOCK: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      		interrupt->Halt();
    	} 

    	PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

		//idk if 0 should be the first argument ugh
    	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

    	return atoi(buffer);
	#endif

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

	#ifdef NETWORK
		PacketHeader outPktHdr;
		MailHeader outMailHdr;

		outPktHdr.from = machineNum;
		outPktHdr.to = 0;

		outMailHdr.from = 1;
		outMailHdr.to = 0;

		std::stringstream ss;
		ss << index;
		std::string indexStr = ss.str();
		std::string destroyLock = "2 ";

		std::string message = destroyLock + indexStr;
		outMailHdr.length = strlen(message.c_str()) + 1;
		cout << "Destroy Lock: Sending message: " << message << endl;
		bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

		 if ( !success ) {
      		printf("DESTROY LOCK: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      		interrupt->Halt();
    	} 

    	PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

		//imo we should have a receive here just to make sure the action finishes on the server b4 returning
    	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

    	return;
	#endif




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

#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << buf;
	std::string indexStr = ss.str();
	std::string createCondition = "5 ";

	std::string message = createCondition + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Create Condition: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("CREATE CONDITION: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return atoi(buffer);
#endif
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
#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << index;
	std::string indexStr = ss.str();
	std::string destroyCondition = "6 ";

	std::string message = destroyCondition + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Destroy Condition: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("DESTROY CONDITION: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return;
#endif
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

int CreateMonitor_Syscall(int lockIndex, int conditionIndex, int maxIndex) {
	kernelMonitorLock.Acquire();
	#ifdef NETWORK
		PacketHeader outPktHdr;
		MailHeader outMailHdr;

		outPktHdr.from = machineNum;
		outPktHdr.to = 0;

		outMailHdr.from = 1;
		outMailHdr.to = 0;

		std::stringstream ss;
		ss << lockIndex;
		std::string lockStr = ss.str();

		ss.clear();
		ss.str("");

		ss << conditionIndex;
		std::string conditionStr = ss.str();

		ss.clear();
		ss.str("");

		ss << maxIndex;
		std::string maxStr = ss.str();

		std::string createMonitor = "10 ";

		std::string message = createMonitor + lockStr + conditionStr + maxStr;
		outMailHdr.length = strlen(message.c_str()) + 1;
		cout << "Create Monitor: Sending message: " << message << endl;
		bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

		 if ( !success ) {
      		printf("CREATE MONITOR: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      		interrupt->Halt();
    	} 

    	PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

		//idk if 0 should be the first argument ugh
    	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    	kernelMonitorLock.Release();
    	return atoi(buffer);
	#endif
	kernelMonitorLock.Release();
	//should only be for part 3 stuff
	return -1;
}

void DestroyMonitor_Syscall(int monitorIndex) {
	kernelLockLock.Acquire();

	#ifdef NETWORK
		PacketHeader outPktHdr;
		MailHeader outMailHdr;

		outPktHdr.from = machineNum;
		outPktHdr.to = 0;

		outMailHdr.from = 1;
		outMailHdr.to = 0;

		std::stringstream ss;
		ss << monitorIndex;
		std::string indexStr = ss.str();
		std::string destroyMonitor = "11 ";

		std::string message = destroyMonitor + indexStr;
		outMailHdr.length = strlen(message.c_str()) + 1;
		cout << "Destroy Monitor: Sending message: " << message << endl;
		bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

		 if ( !success ) {
      		printf("DESTROY MONITOR: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      		interrupt->Halt();
    	} 

    	PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

		//imo we should have a receive here just to make sure the action finishes on the server b4 returning
    	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    	kernelMonitorLock.Release();
    	return;
	#endif

    kernelMonitorLock.Release();

}


int GetMonitor_Syscall(int monitorIndex) {
	int conditionIndex = -1;
	int lockIndex = -1;
	int retVal = 0;

#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << monitorIndex;
	std::string indexStr = ss.str();
	std::string getMonitor = "12 ";

	std::string message = getMonitor + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Get Monitor: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("GET MONITOR: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return atoi(buffer);
#endif

	kernelMonitorLock.Acquire();
	if (monitorIndex < 0 || monitorIndex >= kernelMonitorIndex) {
		cout << "Invalid monitor index" << endl;
		kernelMonitorLock.Release();
		return 0;
	}
	if (kernelMonitorList[monitorIndex].monitor == NULL) {
		printf("No monitor at index %i.\n", monitorIndex);
		kernelMonitorLock.Release();
		return 0;
	}

	if (kernelMonitorList[monitorIndex].addrsp != currentThread->space) {
		printf("Monitor %i does not belong to the process.\n",
				monitorIndex);
		kernelMonitorLock.Release();
		return 0;
	}

	kernelConditionLock.Acquire();
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
		kernelConditionLock.Release();
		return 0;
	}

	lockIndex = kernelMonitorList[monitorIndex].monitor->lock;
	conditionIndex = kernelMonitorList[monitorIndex].monitor->condition;

	// Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to broadcast.\n");
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return 0;
	}

	DEBUG('t', "Getting Monitor [%i].\n", monitorIndex);
	retVal = kernelMonitorList[monitorIndex].monitor->number;
	kernelLockLock.Release();
	kernelConditionLock.Release();
	kernelMonitorLock.Release();
	return retVal;
}

int SetMonitor_Syscall(int monitorIndex, int value) {
	int conditionIndex = -1;
	int lockIndex = -1;

#ifdef NETWORK
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = machineNum;
	outPktHdr.to = 0;

	outMailHdr.from = 1;
	outMailHdr.to = 0;

	std::stringstream ss;
	ss << monitorIndex;
	std::string indexStr = ss.str();
	std::string getMonitor = "13 ";

	std::string message = getMonitor + indexStr;
	outMailHdr.length = strlen(message.c_str()) + 1;
	cout << "Set Monitor: Sending message: " << message << endl;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

	 if ( !success ) {
  		printf("SET MONITOR: The Client Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
  		interrupt->Halt();
	}

	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);

	return atoi(buffer);
#endif

	kernelMonitorLock.Acquire();
	if (monitorIndex < 0 || monitorIndex >= kernelMonitorIndex) {
		cout << "Invalid monitor index" << endl;
		kernelMonitorLock.Release();
		return 0;
	}
	if (kernelMonitorList[monitorIndex].monitor == NULL) {
		printf("No monitor at index %i.\n", monitorIndex);
		kernelMonitorLock.Release();
		return 0;
	}

	if (kernelMonitorList[monitorIndex].addrsp != currentThread->space) {
		printf("Monitor %i does not belong to the process.\n", monitorIndex);
		kernelMonitorLock.Release();
		return 0;
	}

	kernelConditionLock.Acquire();
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
		kernelConditionLock.Release();
		return 0;
	}

	lockIndex = kernelMonitorList[monitorIndex].monitor->lock;
	conditionIndex = kernelMonitorList[monitorIndex].monitor->condition;

	// Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to broadcast.\n");
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return 0;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();
		kernelConditionLock.Release();
		return 0;
	}

	DEBUG('t', "Setting Monitor [%i].\n", monitorIndex);
	kernelMonitorList[monitorIndex].monitor->number = value;
	kernelLockLock.Release();
	kernelConditionLock.Release();
	kernelMonitorLock.Release();
	return 0;
}

#ifndef NETWORK
int handleMemoryFull() {
	//--- Find PPN to evict --//
	// if random
	int evictPPN = 0;
	int swapWriteLoc = -1;
	bool valid = false;

	iptLock.Acquire();
	if (!isFIFO) {
		evictPPN = rand() % NumPhysPages;
	}

	// if FIFO
	else if (isFIFO) {
		evictPPN = pageQueue.pop();
//		DEBUG('v', "\tPopped VPN %i to evict\n", vpn);
	}

	//-- Got a PPN to evict now --//
	// update TLB

	for (int i = 0; i < TLBSize; ++i) {
		if (evictPPN == machine->tlb[i].physicalPage && machine->tlb[i].valid){
			ipt[evictPPN].dirty = machine->tlb[i].dirty;
			machine->tlb[i].valid = FALSE;
		}
	}

	DEBUG('v', "\tSelected [PPN %i ==> VPN %i] to evict\n", evictPPN, ipt[evictPPN].virtualPage);

	//-- Write to Swap --//
	swapLock.Acquire();
	if (swapFile == NULL) {
		swapFile = fileSystem->Open(swapFileName);
	}
	// change to some sort of handling
	ASSERT(swapFile != NULL);

	// page replacement
	if (ipt[evictPPN].dirty) {
		// update page table
		swapWriteLoc = swapFileBM.Find();
		int evictVPN = ipt[evictPPN].virtualPage;

		// handling
		if (swapWriteLoc < 0) {
			DEBUG('v', "\tSWAP OUT OF SPACE!!!\n");
		}
		else {
			DEBUG('v', "\tWriting dirty page [PPN %i][VPN %i] to swap: from main memory %i, size %i, write loc %i,  byte offset %i\n",
					evictPPN, ipt[evictPPN].virtualPage, evictPPN*PageSize, PageSize, swapWriteLoc, swapWriteLoc * PageSize);
			swapFile->WriteAt(&machine->mainMemory[evictPPN * PageSize],
					PageSize, swapWriteLoc * PageSize);

			pageTableLock.Acquire();
			ipt[evictPPN].space->getPageTable()[evictVPN].physicalPage = -1;
			ipt[evictPPN].space->getPageTable()[evictVPN].diskLocation =
					PageTableEntry::SWAP;
			DEBUG('v', "\t\t[VPN %i] disk location set to %d == %d\n", evictVPN, ipt[evictPPN].space->getPageTable()[evictVPN].diskLocation, PageTableEntry::SWAP);
			DEBUG('v', "\t\t[VPN %i] disk location addr is %#x\n",evictVPN, &ipt[evictPPN].space->getPageTable()[evictVPN].diskLocation);
			ipt[evictPPN].space->getPageTable()[evictVPN].swapOffset = swapWriteLoc * PageSize;
			pageTableLock.Release();
		}
	}
	swapLock.Release();
	iptLock.Release();
	// return ppn for handleIPTMiss to populate IPT
	return evictPPN;
}

int handleIPTMiss(int vpn) {
	//-- Find a place to put the page --//
	int ppn = bitmap.Find();
	if (ppn == -1) {
		DEBUG('v', "\tMemory is full!!!\n");
		ppn = handleMemoryFull();
		// swap file shit
	}

	//--- Look in page table for VPN --//
	pageTableLock.Acquire();
	PageTableEntry * pte = &currentThread->space->getPageTable()[vpn];
	DEBUG('v', "\t\t[VPN %i] disk location is currently to %d\n", vpn, currentThread->space->getPageTable()[vpn].diskLocation);
	DEBUG('v', "\t\t[VPN %i] disk location addr is %#x\n", vpn, &currentThread->space->getPageTable()[vpn].diskLocation);
	//--- Find location of VP, swap or exe, load in memory, update page tables --//
	//--- EXECUTABLE --//
	if (pte->diskLocation == PageTableEntry::EXECUTABLE) {
		DEBUG('v', "\t[VPN %i] is in EXE, reading to [main memory PPN %i, addr %i] from EXE %#x at [byte offset %i]\n",
				vpn, ppn, PageSize * ppn, currentThread->space->myExecutable, pte->byteOffset);
		// load into memory
		ASSERT(pte->byteOffset/PageSize >= 0);
		currentThread->space->myExecutable->ReadAt(
				&(machine->mainMemory[PageSize * ppn]),
				PageSize, pte->byteOffset);
		// not dirty because freshly loaded
		pte->dirty = FALSE;
	}
	//-- SWAP --//
	else if (pte->diskLocation == PageTableEntry::SWAP) {
		swapLock.Acquire();
		DEBUG('v', "\t[VPN %i] is in SWAP, reading to [main memory PPN %i, addr %i] from SWAP %#x at [byte offset %i]\n",
						vpn, ppn, PageSize * ppn, swapFile, pte->swapOffset);
		ASSERT(swapFile != NULL);
		ASSERT(pte->swapOffset/PageSize >= 0);
		swapFile->ReadAt(&machine->mainMemory[ppn * PageSize], PageSize, pte->swapOffset);
		swapFileBM.Clear(pte->swapOffset/PageSize);
		DEBUG('v', "\tClearing swap page %i\n", pte->swapOffset/PageSize);

		pte->swapOffset = -1;
		pte->diskLocation = PageTableEntry::MEMORY;
		pte->dirty = TRUE;

		swapLock.Release();
	}
	else if (pte->diskLocation == PageTableEntry::MEMORY) {
		DEBUG('v', "\t[VPN %i] is in MEMORY.\n", vpn);
	}
	else {
		DEBUG('v', "\t[VPN %i] is in WRONG.\n", vpn);
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
	ipt[ppn].swapOffset = pte->swapOffset;
	ipt[ppn].diskLocation = pte->diskLocation;
	iptLock.Release();
	pageTableLock.Release();

	return ppn;
}

int handlePageFault() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	int vpn = machine->ReadRegister(BadVAddrReg) / PageSize;
	DEBUG('v', "\tMissed VPN %i\n", vpn);
	int ppn = -1;

	//--- Look in IPT for needed VPN ---//
	iptLock.Acquire();
	for (int i = 0; i < NumPhysPages; ++i) {
		if (vpn == ipt[i].virtualPage && ipt[i].valid
				&& ipt[i].space == currentThread->space) {
			ppn = i;
			ipt[i].use = TRUE;
			DEBUG('v', "\tFound [VPN %i] in IPT at [PPN %i == %i]\n", ipt[i].virtualPage, ppn, ipt[i].physicalPage);
			break;
		}
	}
	iptLock.Release();

	//--- Not found in IPT (IPT miss) --//
	if (ppn < 0 || ppn >= NumPhysPages) {
		DEBUG('v', "\tIPT miss\n");
		ppn = handleIPTMiss(vpn);
	}
	DEBUG('v', "\tTried PT, PPN is %i\n", ppn);

	if (ppn < 0 || ppn >= NumPhysPages) {
		DEBUG('v', "\tVPN %i does not exist...\n", vpn);
		interrupt->Halt();
	}
	//--- Continue after IPT miss or not IPT miss --//
	//-- VP is now in IPT by now --//
	//--- Update TLB, compute PA --//
	else {

		if(machine->tlb) {
			// Must update IPT if tlb entry not dirty and is valid
			if (machine->tlb[currentTLBEntry].dirty
					&& machine->tlb[currentTLBEntry].valid) {
				DEBUG('v', "\tUpdating IPT and PT because TLB is dirty and valid\n");
				int dirtyppn = machine->tlb[currentTLBEntry].physicalPage;
				ipt[dirtyppn].dirty = TRUE;
				int dirtyvpn = machine->tlb[currentTLBEntry].virtualPage;
				currentThread->space->getPageTable()[dirtyvpn].dirty = TRUE;
			}

			iptLock.Acquire();
			IPTEntry entry = ipt[ppn];
			DEBUG('v', "\tPPN is %i\n", ppn);
			DEBUG('v', "\tPutting in TLB at %i\n", currentTLBEntry);
			machine->tlb[currentTLBEntry].physicalPage = entry.physicalPage;
			machine->tlb[currentTLBEntry].virtualPage = entry.virtualPage;
			machine->tlb[currentTLBEntry].valid = entry.valid;
			machine->tlb[currentTLBEntry].dirty = entry.dirty;
			machine->tlb[currentTLBEntry].use = entry.use;
			machine->tlb[currentTLBEntry].readOnly = entry.readOnly;
			currentTLBEntry = (currentTLBEntry + 1) % TLBSize;
			iptLock.Release();
		}
		if (isFIFO) pageQueue.push(vpn);
	}

	(void) interrupt->SetLevel(oldLevel);
	return ppn;
}
#endif

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
			printf("CreateLock syscall.\n");
			rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_DestroyLock:
			DEBUG('a', "DestroyLock syscall.\n");
			DestroyLock_Syscall(machine->ReadRegister(4));
			break;
		case SC_CreateCondition:
			DEBUG('a', "CreateCondition syscall.\n");
			rv = CreateCondition_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_DestroyCondition:
			DEBUG('a', "DestroyCondition syscall.\n");
			DestroyCondition_Syscall(machine->ReadRegister(4));
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
		case SC_CreateMonitor:
			DEBUG('a', "CreateMonitor syscall.\n");
			CreateMonitor_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
			break;
		case SC_DestroyMonitor:
			DEBUG('a', "DestroyMonitor syscall.\n");
			DestroyMonitor_Syscall(machine->ReadRegister(4));
			break;
		case SC_SetMonitor:
			DEBUG('a', "SetMonitor syscall.\n");
			SetMonitor_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_GetMonitor:
			DEBUG('a', "GetMonitor syscall.\n");
			GetMonitor_Syscall(machine->ReadRegister(4));
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
#ifdef NETWORK
		DEBUG('v', "Page Fault Exception for thread id %i, process id %i\n", currentThread->threadIndex, currentThread->space->processIndex);
				interrupt->Halt();
#else
			DEBUG('v', "Page Fault Exception for thread id %i, process id %i\n", currentThread->threadIndex, currentThread->space->processIndex);
		int ppn = handlePageFault();
#endif
	}
	else {
		cout << "Unexpected user mode exception - which:" << which << "  type:"
				<< type << endl;
		interrupt->Halt();
	}
}

