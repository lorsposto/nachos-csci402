/*
 * passport_office.cc
 *
 *  Created on: Sep 11, 2015
 *      Author: LorraineSposto
 */
#include "utility.h"
#include "system.h"
#include "synch.h"
#include <time.h>
#include <stdlib.h>

#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

struct ClerkLine {
public:
	enum clerkType {
		APP, PIC, PP
	};
	enum clerkState {
		OPEN, BUSY, BREAK
	};
	char * name;
	int lineCount;
	clerkType type;
	clerkState state;
	Condition * cv;

	ClerkLine() {
		name = NULL;
		lineCount = 0;
		state = OPEN;
		type = APP;
		cv = new Condition(name);
	}

	ClerkLine(char * n, clerkType t) {
		name = n;
		lineCount = 0;
		state = OPEN;
		type = t;
		cv = new Condition(name);
	}
};

struct Customer {
	int index;
	bool picDone;
	bool appDone;

	Customer(int i) {
		index = i;
		picDone = false;
		appDone = false;
	}
};

// Customer's first step, either app or pic clerk
Lock picappLineLock("App Pic Line Lock");
Lock pictureLineLock("Picture Line Lock");
Lock passportLineLock("Passpork Line Lock");

ClerkLine * clerkLines[2];

Customer * customers[2];

int randomYield() {
	return rand()%80 + 20;
}

void doingShit() {
	int r = randomYield();
	printf("%s doing shit for %i.\n", currentThread->getName(), r);
	for (int i=0; i < r; ++i) {
		currentThread->Yield();
	}
}

// The customer functionality
void beCustomer(int index) {
	//-------------------------------------------------------------------
	// Step 1: choosing shortest of App and Pic line
	//-------------------------------------------------------------------
//	bool picDone = false;
//	bool appDone = false;
	int myLine;
	int lineSize;
	Lock * targetLock;

	printf("%s starting customer process.\n", currentThread->getName());
	while (!customers[index]->picDone || !customers[index]->appDone) {
		myLine = -1;
		lineSize = 1000;

		if (!customers[index]->picDone || !customers[index]->appDone) {
			// can choose either pic or app line
			if (!customers[index]->picDone && !customers[index]->appDone) {
				picappLineLock.Acquire();
				printf(
						"%s acquired %s, about to choose shortest of App or Pic clerk\n",
						currentThread->getName(), picappLineLock.getName());

				for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
					if (clerkLines[i]->lineCount < lineSize
							&& clerkLines[i]->state != ClerkLine::BREAK) {
						myLine = i;
						lineSize = clerkLines[i]->lineCount;
					}
				}

				printf("%s chose Pic/App clerk %i.\n", currentThread->getName(), myLine);
				if (clerkLines[myLine]->type == ClerkLine::APP) {
					customers[index]->appDone = true;
				} else {
					customers[index]->picDone = true;
				}
			} else if (!customers[index]->picDone) { // app already done, has to choose pic line
				picappLineLock.Acquire();
				printf(
						"%s acquired %s, about to choose shortest of App clerk\n",
						currentThread->getName(), picappLineLock.getName());
				for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
					if (clerkLines[i]->type == ClerkLine::PIC
							&& clerkLines[i]->lineCount < lineSize
							&& clerkLines[i]->state != ClerkLine::BREAK) {
						myLine = i;
						lineSize = clerkLines[i]->lineCount;
					}
				}
				printf("%s chose Pic clerk %i.\n", currentThread->getName(), myLine);
				customers[index]->picDone = true;
			} else if (!customers[index]->appDone) { // has to choose app line
				picappLineLock.Acquire();
				printf(
						"%s acquired %s, about to choose shortest of App clerk\n",
						currentThread->getName(), picappLineLock.getName());

				for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
					if (clerkLines[i]->type == ClerkLine::APP
							&& clerkLines[i]->lineCount < lineSize
							&& clerkLines[i]->state != ClerkLine::BREAK) {
						myLine = i;
						lineSize = clerkLines[i]->lineCount;
					}
				}
				printf("%s chose App clerk %i.\n", currentThread->getName(), myLine);
				customers[index]->appDone = true;
			}
		}

//		printf("%s chose clerk %i.\n", currentThread->getName(), myLine);
		if (clerkLines[myLine]->state == ClerkLine::BUSY) {
			// must wait
			clerkLines[myLine]->lineCount++;
			printf("%s waiting for clerk %s.\n", currentThread->getName(),
					clerkLines[myLine]->name);
			clerkLines[myLine]->cv->Wait(&picappLineLock);
			printf("%s done waiting for clerk %s.\n", currentThread->getName(),
					clerkLines[myLine]->name);
			clerkLines[myLine]->lineCount--;
		}

		printf("%s approaching clerk %s.\n", currentThread->getName(),
				clerkLines[myLine]->name);
		clerkLines[myLine]->state = ClerkLine::BUSY;
		printf("%s releasing %s\n", currentThread->getName(),
				picappLineLock.getName());
		picappLineLock.Release();
		doingShit();
	}

	printf("%s finished both tasks.\n", currentThread->getName());
	//-------------------------------------------------------------------
	// Step 2:
	//-------------------------------------------------------------------
}

void PassportOffice() {
	Thread * t;
	char * name;
	int i;

	srand(time(NULL));

	clerkLines[0] = new ClerkLine("Pic Clerk 1", ClerkLine::PIC);
	clerkLines[1] = new ClerkLine("App Clerk 1", ClerkLine::APP);

	for (i = 0; i < 2; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		customers[i] = new Customer(i);
		printf("%s has just entered the passport office.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCustomer, i);
	}

//	for (i=0; i < 2; ++i) {
//		done.P();
//	}
}

