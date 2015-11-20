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

void askOtherServers(Request*, int, void*, void*);
void awaitResponse(int);
void CreateLock(char* lockName, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	int createdLockIndex = -1;

	// check if lock with given name already exists
	for (int i=0; i<kernelLockIndex; i++) {
		if (strcmp(kernelLockList[i].lock->getName(), lockName) == 0) {
			createdLockIndex = i;
			break;
		}
	}

	// PART 4: check if other servers have lock with name
	if (createdLockIndex < 0) {
		Request * r = new Request;
		r->requesterMachineID = inPktHdr.from;
		r->requesterMBID = inMailHdr.from;
		r->requestType = CREATELOCK;
		r->primaryIndex = (void*)lockName;
		r->secondaryIndex = NULL;
		r->noResponses = 0;
		r->status = Request::PENDING;
		requestLock.Acquire();
		requests.push_back(r);
		r->index = requests.size()-1;
		requestLock.Release();
		askOtherServers(r, CREATELOCK, (void *)lockName, NULL);

		while (r->status == Request::PENDING) {
			currentThread->Yield();
		}
		if(r->status == Request::SUCCESS) {
			createdLockIndex = 1;
			return;
		}
	}

	// no lock with given name already exists
	if (createdLockIndex == -1) {
		kernelLockLock.Acquire();
		createdLockIndex = kernelLockIndex + 100*machineNum;
		kernelLockList[kernelLockIndex].lock = new Lock(lockName);
		kernelLockList[kernelLockIndex].addrsp = currentThread->space; // #userprog
		kernelLockList[kernelLockIndex].isToBeDeleted = false;
		// the next new lock's index
		kernelLockIndex++;
		kernelLockLock.Release();
	}

	stringstream ss;
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

int DestroyLock(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, DESTROY LOCK FAILED IN SOME WAY. 
	int reply = -1;

	// check if we have lock
	bool have = false;
	if (machineNum*100 <= index && index < kernelLockIndex + machineNum*100) {
		have = true;
		index = index - machineNum*100;
	}

	// PART 4: check if other servers have lock with name
	if (!have) {
		Request * r = new Request;
		r->requesterMachineID = inPktHdr.from;
		r->requesterMBID = inMailHdr.from;
		r->requestType = DESTROYLOCK;
		r->primaryIndex = (void*)index;
		r->secondaryIndex = NULL;
		r->noResponses = 0;
		r->status = Request::PENDING;
		requestLock.Acquire();
		requests.push_back(r);
		r->index = requests.size()-1;
		requestLock.Release();
		askOtherServers(r, DESTROYLOCK, (void *)index, NULL);

		while (r->status == Request::PENDING) {
			currentThread->Yield();
		}
		if(r->status == Request::SUCCESS) {
			return -1;
		}
	}

	kernelLockLock.Acquire();
	if (index < 0 || index >= kernelLockIndex) {
		// bad index
		printf("Bad lock index to destroy.\n");
		kernelLockLock.Release();

		stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Destroy Lock Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("DESTROY LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
}

int AcquireLock(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	bool have = false;
	if (machineNum*100 <= index && index < kernelLockIndex + machineNum*100) {
		have = true;
		index = index - machineNum*100;
	}

	// PART 4: check if other servers have lock with name
	if (!have) {
		Request * r = new Request;
		r->requesterMachineID = inPktHdr.from;
		r->requesterMBID = inMailHdr.from;
		r->requestType = ACQUIRELOCK;
		r->primaryIndex = (void*)index;
		r->secondaryIndex = NULL;
		r->noResponses = 0;
		r->status = Request::PENDING;
		requestLock.Acquire();
		requests.push_back(r);
		r->index = requests.size()-1;
		requestLock.Release();
		askOtherServers(r, ACQUIRELOCK, (void *)index, NULL);

		while (r->status == Request::PENDING) {
			currentThread->Yield();
		}
		if(r->status == Request::SUCCESS) {
			return -1;
		}
	}

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

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
}

int ReleaseLock(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
{

	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	bool have = false;
	if (machineNum*100 <= index && index < kernelLockIndex + machineNum*100) {
		have = true;
		index = index - machineNum*100;
	}

	// PART 4: check if other servers have lock with name
	if (!have) {
		Request * r = new Request;
		r->requesterMachineID = inPktHdr.from;
		r->requesterMBID = inMailHdr.from;
		r->requestType = RELEASELOCK;
		r->primaryIndex = (void*)index;
		r->secondaryIndex = NULL;
		r->noResponses = 0;
		r->status = Request::PENDING;
		requestLock.Acquire();
		requests.push_back(r);
		r->index = requests.size()-1;
		requestLock.Release();
		askOtherServers(r, RELEASELOCK, (void *)index, NULL);

		while (r->status == Request::PENDING) {
			currentThread->Yield();
		}
		if(r->status == Request::SUCCESS) {
			return -1;
		}
	}

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

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
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
	int createdConditionIndex = kernelConditionIndex + 100*machineNum;
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

int DestroyCondition(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
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

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
}

int WaitCondition(int index, int lockIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
}

int SignalCondition(int index, int lockIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
}

int BroadcastCondition(int index, int lockIndex, PacketHeader inPktHdr, MailHeader inMailHdr)
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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

		return -1;
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
     return index;
}

void CreateMonitor(char* name, int size, PacketHeader inPktHdr, MailHeader inMailHdr)
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
	Monitor * newMonitor = new Monitor(name, size);
	kernelMonitorList[kernelMonitorIndex].monitor = newMonitor;
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

int DestroyMonitor(int index, PacketHeader inPktHdr, MailHeader inMailHdr)
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

		return -1;
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

		return -1;
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
     return index;
}

void GetMonitor(int monitorIndex, int position, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;

	kernelMonitorLock.Acquire();
	if (monitorIndex < 0 || monitorIndex >= kernelMonitorIndex) {
		cout << "Invalid monitor index" << endl;
		kernelMonitorLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Get Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("GET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }

		return;
	}
	if (kernelMonitorList[monitorIndex].monitor == NULL) {
		printf("No monitor at index %i.\n", monitorIndex);
		kernelMonitorLock.Release();
		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Get Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("GET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }
		return;
	}

	if (kernelMonitorList[monitorIndex].addrsp != currentThread->space) {
		printf("Monitor %i does not belong to the process.\n", monitorIndex);
		kernelMonitorLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Get Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("GET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }
		return;
	}

	DEBUG('t', "Getting monitor %i.\n", monitorIndex);
	
	reply = kernelMonitorList[monitorIndex].monitor->getVal(position);

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
//     return monitorIndex;
}

void SetMonitor(int monitorIndex, int position, int value, PacketHeader inPktHdr, MailHeader inMailHdr)
{
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = 0;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = 0;
	outPktHdr.to = inPktHdr.from;

	//THE VALUE WE SEND BACK. IF -1, ACQUIRE LOCK FAILED IN SOME WAY.
	int reply = -1;

	kernelMonitorLock.Acquire();
	if (kernelMonitorList[monitorIndex].monitor == NULL) {
		printf("No monitor at index %i.\n", monitorIndex);
		kernelMonitorLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Set Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("SET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }
		return;
	}

	if (kernelMonitorList[monitorIndex].addrsp != currentThread->space) {
		printf("Monitor %i does not belong to the process.\n", monitorIndex);
		kernelMonitorLock.Release();

		std::stringstream ss;
		ss << reply;
		const char* intStr = ss.str().c_str();

		outMailHdr.length = strlen(intStr) + 1;

	    bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(intStr));
	    cout << "Set Monitor Server: Sending failure message: " << intStr << " to " << outPktHdr.to << ", box " << outMailHdr.to << endl;

	     if ( !success ) {
	      printf("SET MONITOR: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	      interrupt->Halt();
	    }
		return;
	}

	DEBUG('t', "Getting monitor %i.\n", monitorIndex);
	
	kernelMonitorList[monitorIndex].monitor->setVal(position, value);

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
//     return monitorIndex;
}

// code is syscall code, index is resource index if applicable
// pass null if not applicable
void askOtherServers(Request * r, int code, void* arg1, void* arg2) {
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

//	if (code == CREATELOCK || code == CREATECOND) {
//		char * arg2s;
//		if (!arg2) {
//			arg2s = "null";
//		}
//		sprintf(&r->msg, "%i %s %s", code, static_cast<char*>(arg1), arg2s);
//	}
//	else {
//		char * arg2s;
//		if (!arg2) {
//			arg2s = "null";
//		}
//		sprintf(&r->msg, "%i %i %s", code, static_cast<int*>(arg1), arg2s);
//	}

	// search for duplicates
//	for(std::vector<Request *>::iterator it = requests.begin(); it != requests.end(); ++it) {
//		if (strcmp(r->msg, *it->msg) && *it->status == Request::PENDING) {
//			cout << "Found duplicate pending request" << endl;
//			return -1;
//		}
//	}

	for (int i=0; i < NUM_SERVERS; ++i) {
		if (i != machineNum) {
			outMailHdr.from = machineNum;
			outMailHdr.to = i;

			outPktHdr.from = machineNum;
			outPktHdr.to = i;

			std::stringstream ss;
			if (code == CREATELOCK || code == CREATECOND) {
				ss << (static_cast<char*>(arg1));
			}
			else {
				ss << (static_cast<int*>(arg1));
			}
			if (arg2) {
				ss << " " << (static_cast<int*>(arg2));
			}
			std::string indexStr = ss.str(); // aka resource id
			char buf[5];
			sprintf(buf, "%i ", code);
			string codeString(buf);

			string message = "500 " + codeString + indexStr;
			outMailHdr.length = strlen(message.c_str()) + 1;
			cout << "Code " << code << ": Sending message to other server " << i << ": " << message << endl;
			bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));

			 if ( !success ) {
	      		printf("CODE %i to Server %i: The Server query failed. You must not have the other Nachos running. Terminating Nachos.\n", code, i);
	      		interrupt->Halt();
	    	}
			Thread *t = new Thread("Requst", 0);
			t->Fork(awaitResponse, r->index);
//			 awaitResponse(rindex);
		}
	}
}

void awaitResponse(int rindex) {
	PacketHeader inPktHdr;
	MailHeader inMailHdr;
	char buffer[MaxMailSize];

	requestLock.Acquire();
	Request * r;
	std::vector<Request *>::iterator it = requests.begin();
	for(it = requests.begin(); it != requests.end(); ++it) {
		if ((*it)->index == rindex) {
			r = *it;
		}
	}
	requestLock.Release();

	//imo we should have a receive here just to make sure the action finishes on the server b4 returning
	// await response
	postOffice->Receive(machineNum, &inPktHdr, &inMailHdr, buffer);

	// TODO receive
	int rcode = -1;
	int syscode = -1;
	int status = -1;
	cout << "Response: " << buffer << endl;
	stringstream sss;
	sss << buffer;
	sss >> rcode >> syscode >> status;

	requestLock.Acquire();
	if (rcode != SERVERRESPONSE) {
		cout << "MESSAGE RECEIVED NOT A SERVER RESPONSE" << endl;
		ASSERT(FALSE);
	}
	if (syscode != r->requestType) {
		cout << "MESSAGE RECEIVED NOT CREATE LOCK" << endl;
		ASSERT(FALSE);
	}

	if (status > 0)
	{
		r->status = Request::SUCCESS;
		requests.erase(it);
		requestLock.Release();
		return;
	}
	r->noResponses++;
	cout << "No Responses " << r->noResponses << endl;

	if (r->noResponses == NUM_SERVERS - 1) {
		r->status = Request::FAILED;
		cout << "Resource not found at another server" << endl;
		requests.erase(it);
	}
	requestLock.Release();
	return;
}

void receiveServerMsg(char * msg, PacketHeader inPktHdr, MailHeader inMailHdr) {
	stringstream ss;
	ss << msg;
	int requestNumber = -1;
	int primaryIndex = -1;
	int secondaryIndex = -1;
	int tertiaryIndex = -1;
	ss >> requestNumber;

	char name[256];
	if (requestNumber == CREATELOCK || requestNumber == CREATECOND) {
		ss.getline(name, 256);
	} else {
		ss >> primaryIndex;
		ss >> secondaryIndex;
		ss >> tertiaryIndex;
	}
	int index = -1;

	switch(requestNumber) {
		default:
			printf("Unrecognized request: %s\n", msg);
			ASSERT(false);
			break;
		case CREATELOCK:
			printf("Request to Create Lock\n");
			printf("Creating Lock: %s\n", name);
//			CreateLock(name, inPktHdr, inMailHdr);
			index = -1;
			for (int i=0; i<kernelLockIndex; i++) {
				if (strcmp(kernelLockList[i].lock->getName(), name) == 0) {
					index = i;
					break;
				}
			}
			break;
		case DESTROYLOCK:
			printf("Request to Destroy Lock\n");
			printf("Destroying Lock: %i\n", primaryIndex);
			index = DestroyLock(primaryIndex, inPktHdr, inMailHdr);
			break;
		case ACQUIRELOCK:
			printf("Request to Acquire Lock\n");
			printf("Acquiring Lock: %i\n", primaryIndex);
			index = AcquireLock(primaryIndex, inPktHdr, inMailHdr);
			break;
		case RELEASELOCK:
			printf("Request to Release Lock\n");
			printf("Releasing lock: %i\n", primaryIndex);
			index = ReleaseLock(primaryIndex, inPktHdr, inMailHdr);
			break;
		case CREATECOND:
			printf("Request to Create Condition\n");
			printf("Creating condition: %s\n", name);
//			CreateCondition(name, inPktHdr, inMailHdr);
			index = -1;
			for (int i=0; i<kernelConditionIndex; i++) {
				if (strcmp(kernelConditionList[i].condition->getName(), name) == 0) {
					index = i;
					break;
				}
			}
			break;
		case DESTROYCOND:
			printf("Request to Destroy Condition\n");
			printf("Destroying condition: %i\n", primaryIndex);
			index = DestroyCondition(primaryIndex, inPktHdr, inMailHdr);
			break;
		case WAITCOND:
			printf("Request to Wait on Condition\n");
			printf("Waiting on condition: %i; ", primaryIndex);
			printf("with lock: %i\n", secondaryIndex);
			index = WaitCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
			break;
		case SIGNALCOND:
			printf("Request to Signal Condition\n");
			printf("Signaling on condition: %i; ", primaryIndex);
			printf("with lock: %i\n", secondaryIndex);
			index = SignalCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
			break;
		case BROADCASTCOND:
			printf("Request to Broadcast Condition\n");
			printf("Broadcasting on condition: %i; ", primaryIndex);
			printf("with lock: %i\n", secondaryIndex);
			index = BroadcastCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
			break;
		case CREATEMV:
			printf("Request to Create Monitor\n");
			printf("Creating monitor: %i\n", primaryIndex);
//			CreateMonitor(primaryIndex, secondaryIndex, tertiaryIndex, inPktHdr, inMailHdr);
//			index = -1;
//			for (int i=0; i<kernelMonitorIndex; i++) {
//				if (strcmp(kernelMonitorList[i].monitor->getName(), name) == 0) {
//					index = i;
//					break;
//				}
//			}
			break;
		case DESTROYMV:
			printf("Request to Destroy Monitor\n");
			printf("Destroying monitor: %d\n", primaryIndex);
			index = DestroyMonitor(primaryIndex, inPktHdr, inMailHdr);
			break;
		case GETMV:
			printf("Request to Get Monitor\n");
			printf("Getting monitor: %i\n", primaryIndex);
			GetMonitor(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
			break;
		case SETMV:
			printf("Request to Set Monitor\n");
			printf("Setting monitor: %i; ", primaryIndex);
			printf("with value: %i\n", secondaryIndex);
			SetMonitor(primaryIndex, secondaryIndex, tertiaryIndex, inPktHdr, inMailHdr);
			break;
	}
	// send reply to other server
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outMailHdr.from = machineNum;
	outMailHdr.to = inMailHdr.from;

	outPktHdr.from = machineNum;
	outPktHdr.to = inPktHdr.from;

	stringstream sss;
	sss << "501 " << requestNumber << " " << index;
	string message = sss.str();
	cout << "Replying to server query with: " << message << endl;
	outMailHdr.length = strlen(message.c_str()) + 1;
	bool success = postOffice->Send(outPktHdr, outMailHdr, const_cast<char*>(message.c_str()));
	 if ( !success ) {
 		printf("The Server reply failed. You must not have the other Nachos running. Terminating Nachos.\n");
 		interrupt->Halt();
	}
}

//maybe should not be in main.cc
void beServer() {
	int num = 0;
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

    	postOffice->Receive(machineNum, &inPktHdr, &inMailHdr, buffer);
    	num++;
//    	 printf("\tGot \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);

      	int requestNumber = -1;
		int primaryIndex = -1;
		int secondaryIndex = -1;
		int tertiaryIndex = -1;
		char name[256];
		stringstream ss;
		ss << buffer;
		ss.flush();
		ss >> requestNumber;

		if(requestNumber == CREATELOCK || requestNumber == CREATECOND
				|| requestNumber == SERVERMSG) { //creates
			// get name
			ss.getline(name, 256);
		}
		else if (requestNumber == CREATEMV) {
			ss >> primaryIndex;
			ss.getline(name, 256);
		}
		else {
			ss >> primaryIndex;
			ss >> secondaryIndex;
			ss >> tertiaryIndex;
		}
//    	char* temp = buffer;
//    	char* split;
//
//    	split = strtok(temp, " ");

    	//first number
//    	int requestNumber = atoi(split);
    	 // printf("REQUEST NUMBER: %d\n", requestNumber);
//		char name[20];
//		sprintf(name, "ClientReq%i", num);
//		Thread t(name);
		cout << "---" << endl;
    	switch(requestNumber) {
    		default:
    			printf("Unrecognized request: %s\n", buffer);
    			ASSERT(false);
    			break;
    		case CREATELOCK:
    			printf("Request to Create Lock\n");
    			printf("Creating Lock: %s\n", name);
    			CreateLock(name, inPktHdr, inMailHdr);
    			break;
    		case DESTROYLOCK:
    			printf("Request to Destroy Lock\n");
    			printf("Destroying Lock: %i\n", primaryIndex);
    			DestroyLock(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case ACQUIRELOCK:
    			printf("Request to Acquire Lock\n");
    			printf("Acquiring Lock: %i\n", primaryIndex);
    			AcquireLock(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case RELEASELOCK:
    			printf("Request to Release Lock\n");
    			printf("Releasing lock: %i\n", primaryIndex);
    			ReleaseLock(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case CREATECOND:
    			printf("Request to Create Condition\n");
    			printf("Creating condition: %s\n", name);
    			CreateCondition(name, inPktHdr, inMailHdr);
    			break;
    		case DESTROYCOND:
    			printf("Request to Destroy Condition\n");
    			printf("Destroying condition: %i\n", primaryIndex);
    			DestroyCondition(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case WAITCOND:
    			printf("Request to Wait on Condition\n");
    			printf("Waiting on condition: %i; ", primaryIndex);
    			printf("with lock: %i\n", secondaryIndex);
    			WaitCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
    		case SIGNALCOND:
    			printf("Request to Signal Condition\n");
    			printf("Signaling on condition: %i; ", primaryIndex);
    			printf("with lock: %i\n", secondaryIndex);
    			SignalCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
    		case BROADCASTCOND:
    			printf("Request to Broadcast Condition\n");
    			printf("Broadcasting on condition: %i; ", primaryIndex);
    			printf("with lock: %i\n", secondaryIndex);
    			BroadcastCondition(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
    		case CREATEMV:
    			printf("Request to Create Monitor\n");
    			printf("Creating monitor: %s\n", name);
    			CreateMonitor(name, primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case DESTROYMV:
    			printf("Request to Destroy Monitor\n");
    			printf("Destroying monitor: %d\n", primaryIndex);
    			DestroyMonitor(primaryIndex, inPktHdr, inMailHdr);
    			break;
    		case GETMV:
    			printf("Request to Get Monitor\n");
    			printf("Getting monitor: %i\n", primaryIndex);
    			GetMonitor(primaryIndex, secondaryIndex, inPktHdr, inMailHdr);
    			break;
			case SETMV:
				printf("Request to Set Monitor\n");
				printf("Setting monitor: %i; ", primaryIndex);
    			printf("with value: %i\n", secondaryIndex);
    			SetMonitor(primaryIndex, secondaryIndex, tertiaryIndex, inPktHdr, inMailHdr);
				break;
			case SERVERMSG:
				cout << "Received server message" << endl;
				// here name contains the whole rest of the message besides the 500 code
				// todo fork this
				receiveServerMsg(name, inPktHdr, inMailHdr);
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
		// this now takes an argument to define the number of servers
		if (!strcmp(*argv, "-server")) {
			ASSERT(argc > 1);
			NUM_SERVERS = atoi(*(argv + 1));
			cout << "The total number of servers is: " << NUM_SERVERS << endl;
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
