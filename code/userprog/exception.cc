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

Lock * locks;

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
	// Check if the number of child threads of this thread exist or not. If they exist then go to sleep until woken
	// up by the last child thread to exit (go in while loop)
	// When you wake up check if this thread (currentthread) has a parent. If it has a parent then reduce the number of
	// children of your parent because you are going to finish. Before calling finish wake up your parent. Then call currentThread->Finish()
	// Now check if it is the main thread of the child process. By main thread of the child process it means that some process has
	// executed this process by calling exec.. Make sure that its not the main process/thread (first one actual main)
	// Now tell your children (if they exist, use your Process Table for this purpose) that you are going to finish.
	// More over you need to tell your parent that your are dying, so you need to remove your entry from the process table
	// of your parent. In case  your parent has called join upon you then you need to wake him up, otherwise if it has not called
	// join then just remove your identity from your parents process table and decrement the number of childprocess counter of your
	// parent process table and delete your addresspace. If it is the main thread of the child process call currentthread finish
}

/* Helper function for Exec */
void exec_thread(int vaddr) {
	// Initialize the register by using currentThread->space
	// Call Restore State through currentThread->space
	// Call machine->Run()
}

void Exec_Syscall(int vaddr, int len) {
	// Read the virtual address of the name of the process from the register R4 virtualAddress = machine->ReadRegister(4)
	// Convert it to the physical address and read the contents from there , which will give you the name of the process to be executed
	// Now Open that file using filesystem->Open
	// Store its openfile pointer
	// Create new addrespace for this executable file
	// Create a new thread
	// Allocate the space created to this thread's space
	// Update the process table and related data structures
	// Write the space ID to the register 2
	// Fork the new thread. I call it exec_thread
}

/* Helper function for Fork */
void kernel_thread(int vaddr) {
	// Write to the register PCReg the virtual address
	// Write virtualaddress + 4 in NextPCReg
	// Call Restorestate function inorder to prevent information loss while context switching
	// Write to the stack register, the starting postion of the stack for this thread
	// Call machine->Run()
	machine->Run();
}

void Fork_Syscall(int vaddr/*, int len*/) {
	// Read the virtual address of user function from Register R4 virtualAddress = machine->ReadRegister(4) (?)
	// Create a New thread. This would be a kernel thread
	kernel_thread(vaddr);
	// Update the Process Table for Multiprogramming part
	// Allocate the addrespace to the thread being forked which is essentially current thread's addresspsace
	// because threads share the process addressspace
}

void Yield_Syscall() {
	// Just call currentThread->Yield()
	currentThread->Yield();
}

void Acquire_Syscall(int index) {
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		return;
	}

	if (kernelLockList[index].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread\n.",
				kernelLockList[index].lock->getName());
		return;
	}

	printf("Acquiring lock %s.\n", kernelLockList[index].lock->getName());
	kernelLockList[index].threadsUsing++;
	kernelLockList[index].lock->Acquire();
}

void Release_Syscall(int index) {
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		return;
	}

	if (kernelLockList[index].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[index].lock->getName());
		return;
	}

	printf("Releasing lock %s.\n", kernelLockList[index].lock->getName());
	kernelLockList[index].threadsUsing--;
	kernelLockList[index].lock->Release();
}

void Wait_Syscall(int conditionIndex, int lockIndex) {
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		return;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		return;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
	}

	kernelConditionList[conditionIndex].threadsUsing++;

	// Check lock
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		return;
	}
	kernelLockList[lockIndex].threadsUsing++;

	printf("Condition [%s] waiting on lock [%s].\n",
			kernelConditionList[conditionIndex].condition->getName(),
			kernelLockList[lockIndex].lock->getName());
	kernelConditionList[conditionIndex].condition->Wait(
			kernelLockList[lockIndex].lock);
}

void Signal_Syscall(int conditionIndex, int lockIndex) {
	if (conditionIndex < 0 || conditionIndex >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		return;
	}

	if (kernelConditionList[conditionIndex].condition == NULL) {
		printf("No condition at index %i.\n", conditionIndex);
		return;
	}

	if (kernelConditionList[conditionIndex].addrsp != currentThread->space) {
		printf("Condition %s does not belong to the process.\n",
				kernelConditionList[conditionIndex].condition->getName());
	}

	kernelConditionList[conditionIndex].threadsUsing--;

	// Check lock
	if (lockIndex < 0 || lockIndex >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[lockIndex].lock->getName());
		return;
	}
	kernelLockList[lockIndex].threadsUsing--;

	printf("Condition [%s] signaling on lock [%s].\n",
			kernelConditionList[conditionIndex].condition->getName(),
			kernelLockList[lockIndex].lock->getName());
	kernelConditionList[conditionIndex].condition->Signal(
			kernelLockList[lockIndex].lock);
}

void Broadcast_Syscall(int conditionIndex, int lockIndex) {

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
	printf("Creating lock %s.\n", buf);
	int createdLockIndex = kernelLockIndex;
	kernelLockList[kernelLockIndex].lock = new Lock(buf);
	kernelLockList[kernelLockIndex].addrsp = currentThread->space; // #userprog
	kernelLockList[kernelLockIndex].isToBeDeleted = false;
	kernelLockList[kernelLockIndex].threadsUsing = 0;

	// the next new lock's index
	kernelLockIndex++;

	delete[] buf;
	return createdLockIndex;
}

void DestroyLock_Syscall(int index) {
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		return;
	}

	if (kernelLockList[index].threadsUsing > 0) {
		printf("Threads are still using lock %s. Won't destroy.\n",
				kernelLockList[index].lock->getName());
		return;
	}

	if (kernelLockList[index].threadsUsing == 0
			|| kernelLockList[kernelLockIndex].isToBeDeleted) {
		printf("Lock %s will be destroyed.\n",
				kernelLockList[index].lock->getName());
		kernelLockList[kernelLockIndex].isToBeDeleted = true;
		kernelLockList[kernelLockIndex].lock = NULL;
	}
}

int CreateCondition_Syscall(int vaddr, int len) {
	// Create the condition variable with the name in the user buffer pointed to by
	// vaddr.  The condition variable name is at most MAXFILENAME chars long.  No
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

	// Create the new condition variable...
	printf("Creating condition %s.\n", buf);
	int createdConditionIndex = kernelConditionIndex;
	kernelConditionList[kernelConditionIndex].condition = new Condition(buf);
	kernelConditionList[kernelConditionIndex].addrsp = currentThread->space;
	kernelConditionList[kernelConditionIndex].isToBeDeleted = false;
	kernelConditionList[kernelConditionIndex].threadsUsing = 0;
	kernelConditionIndex++;

	delete[] buf;
	return createdConditionIndex;
}

void DestroyCondition_Syscall(int index) {
	if (index < 0 || index >= kernelConditionIndex) {
		printf("Invalid condition index.\n");
		return;
	}

	if (kernelConditionList[index].condition == NULL) {
		printf("No condition at index %i.\n", index);
		return;
	}

	if (kernelConditionList[index].threadsUsing > 0) {
		printf("Threads are still using condition %s. Won't destroy.\n",
				kernelConditionList[index].condition->getName());
		return;
	}

	if (kernelConditionList[index].threadsUsing == 0
			|| kernelConditionList[kernelConditionIndex].isToBeDeleted) {
		printf("Condition %s will be destroyed.\n",
				kernelConditionList[index].condition->getName());
		kernelConditionList[kernelConditionIndex].isToBeDeleted = true;
		kernelConditionList[kernelConditionIndex].condition = NULL;
	}
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
			Fork_Syscall(machine->ReadRegister(4));
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
			Broadcast_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
		case SC_CreateLock:
			DEBUG('a', "CreateLock syscall.\n");
			CreateLock_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
			break;
		case SC_DestroyLock:
			DEBUG('a', "DestroyLock syscall.\n");
			DestroyLock_Syscall(machine->ReadRegister(4));
			break;
		case SC_CreateCondition:
			DEBUG('a', "CreateCondition syscall.\n");
			CreateCondition_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
			break;
		case SC_DestroyCondition:
			DEBUG('a', "DestroyCondition syscall.\n");
			DestroyCondition_Syscall(machine->ReadRegister(4));
			break;
		case SC_Exec:
			DEBUG('a', "DestroyCondition syscall.\n");
			Exec_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;

		}

		// Put in the return value and increment the PC
		machine->WriteRegister(2, rv);
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(PCReg) + 4);
		return;
	}
	else {
		cout << "Unexpected user mode exception - which:" << which << "  type:"
				<< type << endl;
		interrupt->Halt();
	}
}
