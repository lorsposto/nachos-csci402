// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//	  -server sets this machine to be the server
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>  

// External functions used by this file

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);

#ifdef THREADS
extern void Part2(void), TestSuite(void), PassportOffice(void);
#endif


#ifdef NETWORK
void CreateLock(char* lockName, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//this currently does not prevent locks with the same name
	kernelLockLock.Acquire();
	int createdLockIndex = kernelLockIndex;
	kernelLockList[kernelLockIndex].lock = new Lock(lockName);
	kernelLockList[kernelLockIndex].addrsp = currentThread->space; // #userprog
	kernelLockList[kernelLockIndex].isToBeDeleted = false;
	// the next new lock's index
	kernelLockIndex++;
	kernelLockLock.Release();

	std::stringstream ss;
	ss << createdLockIndex;
	const char* intStr = ss.str().c_str();

	outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;

    outMailHdr.length = strlen(intStr) + 1;
    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Create Lock Server: Sending message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("CREATE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

void DestroyLock(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, DESTROY LOCK FAILED IN SOME WAY. 
	int reply = -1;

	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to destroy.\n");
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[index].lock->isBusy()) {
		DEBUG('t', "Lock %s is still busy. Won't destroy.\n",
				kernelLockList[index].lock->getName());
		kernelLockList[kernelLockIndex].isToBeDeleted = true;
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

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
	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Destroy Lock Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("DESTROY LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

void AcquireLock(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY. 
	int reply = -1;

	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[index].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread\n.",
				kernelLockList[index].lock->getName());
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}
	kernelLockLock.Release();
	DEBUG('t', "Acquiring lock %s.\n", kernelLockList[index].lock->getName());
	kernelLockList[index].lock->Acquire();

	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Acquire Lock Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

}

void ReleaseLock(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{

	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, RELEASE LOCK FAILED IN SOME WAY. 
	int reply = -1;

	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Release Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("RELEASE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[index].lock == NULL) {
		printf("No lock at index %i.\n", index);
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Release Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("RELEASE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[index].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread.\n",
				kernelLockList[index].lock->getName());
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Release Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("RELEASE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}
	kernelLockLock.Release();
	DEBUG('t', "Releasing lock %s.\n", kernelLockList[index].lock->getName());
	kernelLockList[index].lock->Release();

	if (kernelLockList[index].isToBeDeleted && !kernelLockList[index].lock->isBusy()) {
		kernelLockList[kernelLockIndex].lock = NULL;
	}

	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Release Lock Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("RELEASE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

// -- CONDITIONS -- //
void CreateCondition(char* conditionName, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//this currently does not prevent locks with the same name
	kernelConditionLock.Acquire();
	int createdConditionIndex = kernelConditionIndex;
	kernelConditionList[kernelConditionIndex].condition = new Condition(conditionName);
	kernelConditionList[kernelConditionIndex].addrsp = currentThread->space; // #userprog
	kernelConditionList[kernelConditionIndex].isToBeDeleted = false;
	// the next new lock's index
	kernelConditionIndex++;
	kernelConditionLock.Release();

	std::stringstream ss;
	ss << createdConditionIndex;
	const char* intStr = ss.str().c_str();

	outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;

    outMailHdr.length = strlen(intStr) + 1;
    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Create Condition Server: Sending message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("CREATE CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

void DestroyCondition(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, DESTROY LOCK FAILED IN SOME WAY.
	int reply = -1;

	kernelConditionLock.Acquire();
	if (index < 0 || index >= kernelConditionIndex) {
		// bad index
		printf("Bad condition index to destroy.\n");
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].condition == NULL) {
		printf("No condition at index %i.\n", index);
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	// busy if someone in wait queue
	if (!kernelConditionList[index].condition->isQueueEmpty()) {
		DEBUG('t', "Condition %s is still busy. Won't destroy.\n",
				kernelConditionList[index].condition->getName());
		kernelConditionList[kernelConditionIndex].isToBeDeleted = true;
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

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
	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Destroy Condition Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("DESTROY Condition: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

void WaitCondition(int index, int lockIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;

	kernelConditionLock.Acquire();
	if (index < 0 || index >= kernelConditionIndex) {
		// bad index
		printf("Bad condition index to wait on.\n");
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Wait Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("WAIT CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].condition == NULL) {
		printf("No condition at index %i.\n", index);
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Wait Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("WAIT CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].addrsp != currentThread->space) {
		printf("Condition %s does not belong to current thread\n.",
				kernelConditionList[index].condition->getName());
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Wait Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("WAIT CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	//-- Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread\n.",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}
	kernelLockLock.Release();
	kernelConditionLock.Release();
	DEBUG('t', "Waiting on condition %s.\n", kernelConditionList[lockIndex].condition->getName());
	kernelConditionList[index].condition->Wait(kernelLockList[lockIndex].lock);

	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Wait Condition Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("WAIT CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

}

void SignalCondition(int index, int lockIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;

	kernelConditionLock.Acquire();
	if (index < 0 || index >= kernelConditionIndex) {
		// bad index
		printf("Bad condition index to signal.\n");
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Signal Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("SIGNAL CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].condition == NULL) {
		printf("No condition at index %i.\n", index);
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Signal Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("SIGNAL CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].addrsp != currentThread->space) {
		printf("Condition %s does not belong to current thread\n.",
				kernelConditionList[index].condition->getName());
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Signal Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("SIGNAL CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

		//-- Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread\n.",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}
	kernelLockLock.Release();
	kernelConditionLock.Release();
	DEBUG('t', "Signaling condition %s.\n", kernelConditionList[index].condition->getName());
	kernelConditionList[index].condition->Signal(kernelLockList[lockIndex].lock);

	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Signal Condition Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("SIGNAL CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

void BroadcastCondition(int index, int lockIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;

	kernelConditionLock.Acquire();
	if (index < 0 || index >= kernelConditionIndex) {
		// bad index
		printf("Bad condition index to broadcast.\n");
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Broadcast Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("BROADCAST CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].condition == NULL) {
		printf("No condition at index %i.\n", index);
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Broadcast Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("BROADCAST CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelConditionList[index].addrsp != currentThread->space) {
		printf("Condition %s does not belong to current thread\n.",
				kernelConditionList[index].condition->getName());
		kernelConditionLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Broadcast Condition Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("BROADCAST CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

		//-- Check lock
	kernelLockLock.Acquire();
	if (lockIndex < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to acquire.\n");
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[lockIndex].lock == NULL) {
		printf("No lock at index %i.\n", lockIndex);
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelLockList[lockIndex].addrsp != currentThread->space) {
		printf("Lock %s does not belong to current thread\n.",
				kernelLockList[lockIndex].lock->getName());
		kernelLockLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Acquire Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("ACQUIRE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}
	kernelLockLock.Release();
	kernelConditionLock.Release();
	DEBUG('t', "Waiting on condition %s.\n", kernelConditionList[index].condition->getName());
	kernelConditionList[index].condition->Broadcast(kernelLockList[lockIndex].lock);

	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Broadcast Condition Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("BROADCAST CONDITION: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

}

void CreateMonitor(int lockNum, int conditionNum, int maxNum, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//this currently does not prevent locks with the same name
	kernelMonitorLock.Acquire();
	int createdMonitorIndex = kernelMonitorIndex;
	Monitor newMonitor;
	newMonitor.lock = lockNum;
	newMonitor.condition = conditionNum;
	newMonitor.target = maxNum;
	newMonitor.number = 0;
	kernelMonitorList[kernelMonitorIndex].monitor = &newMonitor;
	kernelMonitorList[kernelMonitorIndex].addrsp = currentThread->space; // #userprog
	kernelMonitorList[kernelMonitorIndex].isToBeDeleted = false;
	// the next new lock's index
	kernelMonitorIndex++;
	kernelMonitorLock.Release();

	std::stringstream ss;
	ss << createdMonitorIndex;
	const char* intStr = ss.str().c_str();

	outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;

    outMailHdr.length = strlen(intStr) + 1;
    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Create Monitor Server: Sending message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("CREATE MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

}

void DestroyMonitor(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, DESTROY LOCK FAILED IN SOME WAY. 
	int reply = -1;

	kernelMonitorLock.Acquire();
	if (index < 0 || index >= kernelMonitorIndex) {
		// bad index
		printf("Bad monitor index to destroy.\n");
		kernelMonitorLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelMonitorList[index].monitor == NULL) {
		printf("No monitor at index %i.\n", index);
		kernelMonitorLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}

	if (kernelMonitorList[kernelMonitorIndex].isToBeDeleted) {
		DEBUG('t', "Monitor will be destroyed.\n");
		kernelMonitorList[kernelMonitorIndex].isToBeDeleted = true;
		kernelMonitorList[kernelMonitorIndex].monitor = NULL;
	}
	kernelMonitorLock.Release();
	reply = 1;

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Destroy Monitor Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("DESTROY MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }
}

void GetMonitor(int monitorIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;
	int lockIndex;
	int conditionIndex;

	kernelMonitorLock.Acquire();
	if (monitorIndex < 0 || monitorIndex >= kernelMonitorIndex) {
		cout << "Invalid monitor index" << endl;
		kernelMonitorLock.Release();
		return;
	}
	if (kernelMonitorList[monitorIndex].monitor == NULL) {
		printf("No monitor at index %i.\n", monitorIndex);
		kernelMonitorLock.Release();
		return;
	}

	if (kernelMonitorList[monitorIndex].addrsp != currentThread->space) {
		printf("Monitor %i does not belong to the process.\n", monitorIndex);
		kernelMonitorLock.Release();
		return;
	}

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

	lockIndex = kernelMonitorList[monitorIndex].monitor->lock;
	conditionIndex = kernelMonitorList[monitorIndex].monitor->condition;

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
	kernelLockLock.Release();
	kernelConditionLock.Release();
	DEBUG('t', "Getting monitor %i.\n", monitorIndex);
	
	// --- FUNCTIONALITY
	kernelLockList[lockIndex].lock->Acquire();
	while (kernelMonitorList[monitorIndex].monitor->number != kernelMonitorList[monitorIndex].monitor->number) {
		kernelConditionList[conditionIndex].condition->Wait(kernelLockList[lockIndex].lock);
	}

	reply = kernelMonitorList[monitorIndex].monitor->number;
	kernelConditionList[conditionIndex].condition->Signal(kernelLockList[lockIndex].lock);

	kernelLockList[lockIndex].lock->Release();
	// ---

	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Get Monitor Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("GET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

}

void SetMonitor(int monitorIndex, int value, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;
	int lockIndex;
	int conditionIndex;

	kernelMonitorLock.Acquire();
	if (monitorIndex < 0 || monitorIndex >= kernelMonitorIndex) {
		cout << "Invalid monitor index" << endl;
		kernelMonitorLock.Release();
		return;
	}
	if (kernelMonitorList[monitorIndex].monitor == NULL) {
		printf("No monitor at index %i.\n", monitorIndex);
		kernelMonitorLock.Release();
		return;
	}

	if (kernelMonitorList[monitorIndex].addrsp != currentThread->space) {
		printf("Monitor %i does not belong to the process.\n", monitorIndex);
		kernelMonitorLock.Release();
		return;
	}

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

	lockIndex = kernelMonitorList[monitorIndex].monitor->lock;
	conditionIndex = kernelMonitorList[monitorIndex].monitor->condition;

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
	kernelLockLock.Release();
	kernelConditionLock.Release();
	DEBUG('t', "Getting monitor %i.\n", monitorIndex);
	
	// --- FUNCTIONALITY wtf is this doing
	kernelLockList[lockIndex].lock->Acquire();
	while (kernelMonitorList[monitorIndex].monitor->number == kernelMonitorList[monitorIndex].monitor->number) {
		kernelConditionList[conditionIndex].condition->Wait(kernelLockList[lockIndex].lock);
	}

	kernelMonitorList[monitorIndex].monitor->number = value;
	kernelConditionList[conditionIndex].condition->Signal(kernelLockList[lockIndex].lock);

	kernelLockList[lockIndex].lock->Release();
	// ---

	reply = 1;
	std::stringstream ss;
	ss << reply;
	const char* intStr = ss.str().c_str();

	outMailHdr.length = strlen(intStr) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
    cout << "Set Monitor Server: Sending success message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

     if ( !success ) {
      printf("SET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

}

//maybe should not be in main.cc
void beServer() {
	/*while(true) {
	 1.	Wait for request msg
	 2.	Parse request
	 3.	Validate args
	 4.	Do the work
	 5.	If cient can continue – send reply else queue reply (???)
	 }*/
	while(true) {

		PacketHeader inPktHdr;
		MailHeader inMailHdr;
		char buffer[MaxMailSize];

    	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    	printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);

      	int requestNumber = -1;
		int primaryIndex = -1;
		int secondaryIndex = -1;
		int tertiaryIndex = -1;
		char name[256];
		std::stringstream ss;
		ss << buffer;
		ss.flush();
		ss >> requestNumber;

		if(requestNumber == 1 || requestNumber == 5 || requestNumber == 10) { //creates
			// get name
			ss.getline(name, 256);
		}
		else {
			ss >> primaryIndex;
			ss >> secondaryIndex;
			ss >> tertiaryIndex;
		}
    	char* temp = buffer;
    	char* split;

    	split = strtok(temp, " ");

    	//first number
//    	int requestNumber = atoi(split);
    	printf("REQUEST NUMBER: %d\n", requestNumber);


    	switch(requestNumber) {
    		default:
    			printf("Unrecognized request: %s\n", split);
    			ASSERT(false);
    			break;
    		case 1:
    			printf("Request to Create Lock\n");
    			printf("LOCK NAME: %s\n", name);
    			CreateLock(name, inPktHdr, inMailHdr);
    			break;
    		case 2:
    			printf("Request to Destroy Lock\n");
    			printf("LOCK DESTROY INDEX: %i\n", primaryIndex);
    			DestroyLock(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 3:
    			printf("Request to Acquire Lock\n");
    			printf("LOCK ACQUIRE INDEX: %i\n", primaryIndex);
    			AcquireLock(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 4:
    			printf("Request to Release Lock\n");
    			printf("LOCK RELEASE INDEX: %i\n", primaryIndex);
    			ReleaseLock(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 5:
    			printf("Request to Create Condition\n");
    			printf("CREATE CONDITION NAME: %s\n", name);
    			CreateCondition(name, inPktHdr, inMailHdr);
    			break;
    		case 6:
    			printf("Request to Destroy Condition\n");
    			printf("DESTROY CONDITION INDEX: %i\n", primaryIndex);
    			DestroyCondition(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 7:
    			printf("Request to Wait on Condition\n");
    			printf("CONDITION WAIT INDEX: %i\n", primaryIndex);
    			printf("LOCK WAIT INDEX: %i\n", secondaryIndex);
    			WaitCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 8:
    			printf("Request to Signal Condition\n");
    			printf("CONDITION SIGNAL INDEX: %i\n", primaryIndex);
    			printf("LOCK SIGNAL INDEX: %i\n", secondaryIndex);
    			SignalCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 9:
    			printf("Request to Broadcast Condition\n");
    			printf("CONDITION BROADCAST INDEX: %i\n", primaryIndex);
    			printf("LOCK BROADCAST INDEX: %i\n", secondaryIndex);
    			BroadcastCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
    		case 10:
    			printf("Request to Create Monitor\n");
    			printf("CREATE MONITOR NAME: %s\n", name);
    			CreateMonitor(primaryIndex, secondaryIndex, tertiaryIndex, inPktHdr, inMailHdr);
    		case 11:
    			printf("Request to Destroy Monitor\n");
    			printf("DESTROY MONITOR INDEX: %d\n", primaryIndex);
    			DestroyMonitor(primaryIndex, inPktHdr, inMailHdr);
    		case 12:
    			printf("Request to Get Monitor\n");
    			printf("GET MONITOR INDEX: %i\n", primaryIndex);
    			GetMonitor(primaryIndex, inPktHdr, inMailHdr);
    			break;
			case 13:
				printf("Request to Broadcast Condition\n");
				printf("SET MONITOR INDEX: %i\n", primaryIndex);
    			printf("SET MONITOR VALUE: %i\n", secondaryIndex);
    			SetMonitor(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
				break;
    	}

    	//"if client can continue, send reply else queue reply?"
	}
}
#endif

//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int main(int argc, char **argv) {
	int argCount;			// the number of arguments
	// for a particular command

	DEBUG('t', "Entering main");
	(void) Initialize(argc, argv);

#ifdef THREADS
	// ThreadTest();
//    TestSuite();
	PassportOffice();
#endif

	for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
		argCount = 1;
		if (!strcmp(*argv, "-z"))               // print copyright
			printf(copyright);
#ifdef THREADS
		if (!strcmp(*argv, "-T"))               // Test Suite
		TestSuite();
#endif //THREADS
#ifdef USER_PROGRAM
		if (!strcmp(*argv, "-P")) {
			ASSERT(argc > 1);
			if(!strcmp(*(argv + 1), "FIFO")) {
				isFIFO = true;
			}
			else if(!strcmp(*(argv + 1), "RAND")) {
				isFIFO = false;
			}
			else {
				printf("Unrecognized replacement policy after -P flag\n");
			}
		}
		if (!strcmp(*argv, "-x")) {        	// run a user program
			RandomInit(atoi(*(argv + 1)));
			ASSERT(argc > 1);
			StartProcess(*(argv + 1));
			argCount = 2;
		}
		else if (!strcmp(*argv, "-c")) {      // test the console
			if (argc == 1)
			ConsoleTest(NULL, NULL);
			else {
				ASSERT(argc > 2);
				ConsoleTest(*(argv + 1), *(argv + 2));
				argCount = 3;
			}
			interrupt->Halt();		// once we start the console, then
			// Nachos will loop forever waiting
			// for console input
		}
#endif // USER_PROGRAM
#ifdef FILESYS
		if (!strcmp(*argv, "-cp")) { 		// copy from UNIX to Nachos
			ASSERT(argc > 2);
			Copy(*(argv + 1), *(argv + 2));
			argCount = 3;
		}
		else if (!strcmp(*argv, "-p")) {	// print a Nachos file
			ASSERT(argc > 1);
			Print(*(argv + 1));
			argCount = 2;
		}
		else if (!strcmp(*argv, "-r")) {	// remove Nachos file
			ASSERT(argc > 1);
			fileSystem->Remove(*(argv + 1));
			argCount = 2;
		}
		else if (!strcmp(*argv, "-l")) {	// list Nachos directory
			fileSystem->List();
		}
		else if (!strcmp(*argv, "-D")) {	// print entire filesystem
			fileSystem->Print();
		}
		else if (!strcmp(*argv, "-t")) {	// performance test
			PerformanceTest();
		}
#endif // FILESYS
#ifdef NETWORK
		if (!strcmp(*argv, "-o")) {
			ASSERT(argc > 1);
			Delay(2); 				// delay for 2 seconds
			// to give the user time to
			// start up another nachos
			MailTest(atoi(*(argv + 1)));
			argCount = 2;
		}
		if (!strcmp(*argv, "-server")) {
			beServer();
		}
#endif // NETWORK
	}

	currentThread->Finish();	// NOTE: if the procedure "main"
	// returns, then the program "nachos"
	// will exit (as any other normal program
	// would).  But there may be other
	// threads on the ready list.  We switch
	// to those threads by saying that the
	// "main" thread is finished, preventing
	// it from returning.
	return (0);			// Not reached...
}
