// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"
#include "string.h"

#define QUANTUM 100

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------
void StartProcess(char *filename) {
	int ppn;
	OpenFile *executable = fileSystem->Open(filename);
	AddrSpace *space;

	if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
	}

	processLock.Acquire();
	space = new AddrSpace(executable);
	space->processIndex = processIndex;
	TranslationEntry * pageTable = space->getPageTable();
	for (int i = 0; i < space->numPages; i++) {
		// find a physical page number -L
		ppn = bitmap.Find();
		if (ppn == -1) {
			printf("Nachos is out of memory.\n");
			interrupt->Halt();
		}
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = ppn; // set physical page to the one we found -L
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on
		// a separate page, we could set its
		// pages to be read-only
	}
	space->setPageTable(pageTable);

	process *p = new process;
	p->threadStacks = new int[50]; // max number of threads is 50
	p->numThreadsTotal = 1;
	p->numThreadsRunning = 1; // when does this get incremented???
	p->threadStacks[0] = 0;
	processTable[processIndex] = p;

	processIndex++;
	activeProcesses++;

	currentThread->space = space;
	processLock.Release();

	delete executable;			// close file

	space->InitRegisters();		// set the initial register values
	space->RestoreState();		// load page table register

	machine->Run();			// jump to the user progam
	ASSERT(FALSE);			// machine->Run never returns;
	// the address space exits
	// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) {
	readAvail->V();
}
static void WriteDone(int arg) {
	writeDone->V();
}

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void ConsoleTest(char *in, char *out) {
	char ch;

	console = new Console(in, out, ReadAvail, WriteDone, 0);
	readAvail = new Semaphore("read avail", 0);
	writeDone = new Semaphore("write done", 0);

	for (;;) {
		readAvail->P();		// wait for character to arrive
		ch = console->GetChar();
		console->PutChar(ch);	// echo it!
		writeDone->P();        // wait for write to finish
		if (ch == 'q')
			return;  // if q, quit
	}
}

