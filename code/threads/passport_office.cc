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

struct Customer {
	char * name;
	bool picDone;
	bool appDone;

	Customer(char * n) {
		name = n;
		picDone = false;
		appDone = false;
	}
};

struct Clerk {
public:
	enum clerkType {
		APP, PIC, PP
	};
	enum clerkState {
		AVAILABLE, BUSY, BREAK
	};
	char * name;
	int lineCount;
	clerkType type;
	clerkState state;
	Condition * cv;
	Lock * lock;
//	bool interactionSuccess;

	Customer * customer;

	Clerk() {
		name = NULL;
		lineCount = 0;
		state = AVAILABLE;
		type = APP;
		cv = new Condition(name);
		lock = new Lock(name);
		customer = NULL;
	}

	Clerk(char * n, clerkType t) {
		name = n;
		lineCount = 0;
		state = AVAILABLE;
		type = t;
		cv = new Condition(name);
		lock = new Lock(name);
		customer = NULL;
	}
};

// Customer's first step, either app or pic clerk
Lock picappLineLock("App Pic Line Lock");
Lock pictureLineLock("Picture Line Lock");
Lock passportLineLock("Passpork Line Lock");

const int NUM_CUSTOMERS = 2;

Clerk * clerkLines[2];

Customer * customers[NUM_CUSTOMERS];

int randomYield() {
	return rand() % 80 + 20;
}

void picAppInteraction(int customer, int clerk) {
//	int r = randomYield();
//	printf("%s doing shit for %i.\n", currentThread->getName(), r);
//	for (int i = 0; i < r; ++i) {
//		currentThread->Yield();
//	}

	// Set the clerk's current customer
	clerkLines[clerk]->lock->Acquire();
	clerkLines[clerk]->customer = customers[customer];

	// Do the picture process
	if (clerkLines[clerk]->type == Clerk::PIC) {
		// take picture
		printf("%s approaches clerk %s.\n", currentThread->getName(),
				clerkLines[clerk]->name);
		clerkLines[clerk]->cv->Signal(clerkLines[clerk]->lock);

		// Wait to be shown picture
		// Take picture until he likes it
		while (!customers[customer]->picDone) {
			clerkLines[clerk]->cv->Wait(clerkLines[clerk]->lock);
			if ((rand() % 10) % 2 == 0) {
				printf("%s doesn't like the photo.\n",
						currentThread->getName());
				clerkLines[clerk]->cv->Signal(clerkLines[clerk]->lock);
			}
			else {
				printf("%s likes the photo.\n", currentThread->getName());
				customers[customer]->picDone = true;
				clerkLines[clerk]->cv->Signal(clerkLines[clerk]->lock);
			}
		}
		// customer liked the photo and is leaving
	}

	printf("%s is now leaving %s.\n", currentThread->getName(),
			clerkLines[clerk]->name);
	clerkLines[clerk]->lock->Release();
}

void beClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Picture Clerk
	//-------------------------------------------------------------------
	while (clerkLines[clerkIndex]->type == Clerk::PIC) {
		picappLineLock.Acquire();

		if (clerkLines[clerkIndex]->state == Clerk::AVAILABLE && clerkLines[clerkIndex]->lineCount > 0) {
			printf("%s is available and there is someone in line.\n", clerkLines[clerkIndex]->name);
			clerkLines[clerkIndex]->cv->Signal(&picappLineLock);
			clerkLines[clerkIndex]->state = Clerk::BUSY;
		}
//		else if (clerkLines[clerkIndex]->state == Clerk::AVAILABLE){
//			clerkLines[clerkIndex]->state = Clerk::AVAILABLE;
//		}

		clerkLines[clerkIndex]->lock->Acquire();
		picappLineLock.Release();
		// wait for Customer data
		clerkLines[clerkIndex]->cv->Wait(clerkLines[clerkIndex]->lock);

		// Doing job, customer waiting, signal when done
		while (!clerkLines[clerkIndex]->customer->picDone) {
			printf("%s taking the picture for %s.\n", clerkLines[clerkIndex]->name, clerkLines[clerkIndex]->customer->name);
			clerkLines[clerkIndex]->cv->Signal(clerkLines[clerkIndex]->lock);
			// Waiting for customer to accept photo
			clerkLines[clerkIndex]->cv->Wait(clerkLines[clerkIndex]->lock);
		}
		printf("%s is done at %s. ", clerkLines[clerkIndex]->customer->name, clerkLines[clerkIndex]->name);
		printf("%s is becoming available.\n", clerkLines[clerkIndex]->name);
		clerkLines[clerkIndex]->lock->Release();
	}
	//-------------------------------------------------------------------
	// Application Clerk
	//-------------------------------------------------------------------
//	while (clerkLines[index]->type == ClerkLine::APP) {
//		picappLineLock.Acquire();
//
//		if (clerkLines[index]->lineCount > 0) {
//			clerkLines[index]->cv->Signal(&picappLineLock);
//			clerkLines[index]->state = ClerkLine::BUSY;
//		} else {
//			clerkLines[index]->state = ClerkLine::AVAILABLE;
//		}
//
//		clerkLines[index]->lock->Acquire();
//		picappLineLock.Release();
//		// wait for Customer data
//		clerkLines[index]->cv->Wait(clerkLines[index]->lock);
//		// Doing job, customer waiting, signal when done
//		while(!clerkLines[index]->interactionSuccess) {
//			printf("%s taking the picture.\n", currentThread->getName());
//			clerkLines[index]->cv->Signal(clerkLines[index]->lock);
//			// Waiting for customer to accept photo
//			clerkLines[index]->cv->Wait(clerkLines[index]->lock);
//		}
//		printf("%s is becoming available.\n", currentThread->getName());
//		clerkLines[index]->lock->Release();
//	}
}

// The customer functionality
void beCustomer(int customerIndex) {
	//-------------------------------------------------------------------
	// Step 1: choosing shortest of App and Pic line
	//-------------------------------------------------------------------
	int myLine;
	int lineSize;
	Lock * targetLock;

	printf("%s starting customer process.\n", currentThread->getName());
	while (!customers[customerIndex]->picDone
			&& !customers[customerIndex]->appDone) {
		myLine = -1;
		lineSize = 1000;

		// can choose either pic or app line
		if (!customers[customerIndex]->picDone
				&& !customers[customerIndex]->appDone) {
			picappLineLock.Acquire();
			printf(
					"%s acquired %s, about to choose shortest of App or Pic clerk\n",
					customers[customerIndex]->name, picappLineLock.getName());

			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != Clerk::BREAK) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}
		}
		else if (!customers[customerIndex]->picDone) { // app already done, has to choose pic line
			picappLineLock.Acquire();
			printf("%s acquired %s, about to choose shortest of App clerk\n",
					customers[customerIndex]->name, picappLineLock.getName());
			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->type == Clerk::PIC
						&& clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != Clerk::BREAK) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}
		}
		else if (!customers[customerIndex]->appDone) { // has to choose app line
			picappLineLock.Acquire();
			printf("%s acquired %s, about to choose shortest of App clerk\n",
					currentThread->getName(), picappLineLock.getName());

			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->type == Clerk::APP
						&& clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != Clerk::BREAK) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}
		}

		printf("%s chose %s.\n", customers[customerIndex]->name,
							clerkLines[myLine]->name);
		// Customer must wait for clerk to become available.

		if (clerkLines[myLine]->state == Clerk::BUSY) {
			clerkLines[myLine]->lineCount++;
			printf("%s waiting for %s.\n", customers[customerIndex]->name,
					clerkLines[myLine]->name);
			clerkLines[myLine]->cv->Wait(&picappLineLock);
			printf("%s done waiting for %s.\n", customers[customerIndex]->name,
					clerkLines[myLine]->name);
			clerkLines[myLine]->lineCount--;
		}

		// Clerk is now available, current customer can approach the clerk.
		clerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

		printf("%s acquired lock %s.\n", currentThread->getName(), clerkLines[myLine]->lock->getName());
		clerkLines[myLine]->lock->Acquire();

		printf("%s releasing %s\n", customers[customerIndex]->name,
						picappLineLock.getName());
		picappLineLock.Release();
		// interaction begins
		picAppInteraction(customerIndex, myLine);
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

	clerkLines[0] = new Clerk("Pic Clerk 0", Clerk::PIC);
	clerkLines[1] = new Clerk("App Clerk 1", Clerk::APP);

	for (i = 0; i < 2; ++i) {
		name = clerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beClerk, i);
	}

	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		customers[i] = new Customer(name);
		printf("%s has just entered the passport office.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCustomer, i);
	}

//	for (i=0; i < 2; ++i) {
//		done.P();
//	}
}

