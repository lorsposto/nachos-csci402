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

     if ( !success ) {
      printf("CREATE LOCK: The Server Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
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

    	char* temp = buffer;
    	char* split;

    	split = strtok(temp, " ");

    	//first number
    	int requestNumber = *((int*)split);
    	printf("REQUEST NUMBER: %d\n", requestNumber);


    	switch(requestNumber) {
    		default:
    			printf("Unrecognized request: %s\n", split);
    			ASSERT(false);
    			break;
    		case 1:
    			printf("Request to Create Lock\n");
    			char* lockName = strtok(NULL, " ");
    			printf("LOCK NAME: %s\n", lockName);
    			CreateLock(lockName, inPktHdr, inMailHdr);
    			break;
    		case 2:
    			printf("Request to Destroy Lock\n");
    			break;
    		case 3:
    			printf("Request to Create Condition\n");
    			break;
    		case 4:
    			printf("Request to Destroy Condition\n");
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
