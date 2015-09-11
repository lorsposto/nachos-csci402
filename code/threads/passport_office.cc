/*
 * passport_office.cc
 *
 *  Created on: Sep 11, 2015
 *      Author: LorraineSposto
 */
#include "utility.h"
#include "system.h"
#include "synch.h"

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

// Customer's first step, either app or pic clerk
Lock picappLineLock("App Pic Line Lock");
Lock pictureLineLock("Picture Line Lock");
Lock passportLineLock("Passpork Line Lock");

ClerkLine * clerkLines[2];

// The customer functionality
void beCustomer() {
	//-------------------------------------------------------------------
	// Step 1: choosing shortest of App and Pic line
	//-------------------------------------------------------------------
	bool picDone = false;
	bool appDone = false;

	while (!picDone || !appDone) {
		picappLineLock.Acquire();
		printf("%s acquired %s, about to choose shortest of App or Pic clerk\n",
				currentThread->getName(), picappLineLock.getName());
		int myLine = -1;
		int lineSize = 1000;

		// can choose either pic or app line
		if (!picDone && !appDone) {
			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != ClerkLine::BUSY) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}

			if (clerkLines[myLine]->type == ClerkLine::APP) {
				appDone = true;
			}
			else {
				picDone = true;
			}
			// appp already done, has to choose pic line
		} else if (!picDone) {
			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->type == ClerkLine::PIC
						&& clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != ClerkLine::BUSY) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}
			picDone = true;
			// hs to choose app line
		} else if (!appDone) {
			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->type == ClerkLine::APP
						&& clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != ClerkLine::BUSY) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}
			appDone = true;
		}

		if (clerkLines[myLine]->state == ClerkLine::BUSY) {
			// must wait
			clerkLines[myLine]->lineCount++;
			printf("%s waiting for clerk %s.\n",currentThread->getName(), clerkLines[myLine]->name);
			clerkLines[myLine]->cv->Wait(&picappLineLock);
			printf("%s done waiting for clerk %s.\n",currentThread->getName(), clerkLines[myLine]->name);
			clerkLines[myLine]->lineCount--;
		}
		printf("%s approaching clerk %s.\n",currentThread->getName(), clerkLines[myLine]->name);
		clerkLines[myLine]->state = ClerkLine::BUSY;
		printf("%s releasing %s\n",
				currentThread->getName(), picappLineLock.getName());
		picappLineLock.Release();
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

	Semaphore done("done", 0);

	clerkLines[0] = new ClerkLine("Pic Clerk 1", ClerkLine::PIC);
	clerkLines[1] = new ClerkLine("App Clerk 1", ClerkLine::APP);

	for (i = 0; i < 1; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr) beCustomer, 0);
	}

//	for (i=0; i < 2; ++i) {
//		done.P();
//	}
}

