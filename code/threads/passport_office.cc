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
	Condition * lineCV;
	Condition * transactionCV;
	Lock * transactionLock;

	Customer * customer;

	Clerk() {
		name = NULL;
		lineCount = 0;
		state = AVAILABLE;
		type = APP;
		lineCV = new Condition(name);
		transactionCV = new Condition(name);
		transactionLock = new Lock(name);
		customer = NULL;
	}

	Clerk(char * n, clerkType t) {
		name = n;
		lineCount = 0;
		state = AVAILABLE;
		type = t;
		lineCV = new Condition(name);
		transactionLock = new Lock(name);
		transactionCV = new Condition(name);
		customer = NULL;
	}
};

// Customer's first step, either app or pic clerk
Lock picappLineLock("App Pic Line Lock");
//Lock pictureLineLock("Picture Line Lock");
//Lock passportLineLock("Passpork Line Lock");

const int NUM_CUSTOMERS = 2;

Clerk * clerkLines[1];

Customer * customers[NUM_CUSTOMERS];

int randomYield() {
	return rand() % 80 + 20;
}

void picAppInteraction(int customer, int clerk) {
	// Set the clerk's current customer
	clerkLines[clerk]->transactionLock->Acquire();
	clerkLines[clerk]->customer = customers[customer];
	printf("%s acquired lock %s and set himself as the customer.\n",
			customers[customer]->name,
			clerkLines[clerk]->transactionLock->getName());

	// Do the picture process
	if (clerkLines[clerk]->type == Clerk::PIC) {
		// take picture
		printf("%s approaches clerk %s, signaling to begin interaction.\n",
				currentThread->getName(), clerkLines[clerk]->name);
		clerkLines[clerk]->transactionCV->Signal(
				clerkLines[clerk]->transactionLock);

		// Wait to be shown picture
		// Take picture until he likes it
		while (!customers[customer]->picDone) {
			printf("%s waiting for clerk %s to take the picture.\n",
					currentThread->getName(), clerkLines[clerk]->name);
			clerkLines[clerk]->transactionCV->Wait(
					clerkLines[clerk]->transactionLock);

			if ((rand() % 10) % 2 == 0) {
				printf("%s doesn't like the photo, signaling to clerk %s.\n",
						currentThread->getName(), clerkLines[clerk]->name);
				clerkLines[clerk]->transactionCV->Signal(
						clerkLines[clerk]->transactionLock);
			}
			else {
				printf("%s likes the photo, signaling to clerk %s.\n",
						currentThread->getName(), clerkLines[clerk]->name);
				customers[customer]->picDone = true;
				clerkLines[clerk]->transactionCV->Signal(
						clerkLines[clerk]->transactionLock);
			}
		}
		// customer liked the photo and is leaving
		printf("%s is waiting for the photo to be filed.\n", currentThread->getName());
		clerkLines[clerk]->transactionCV->Wait(
								clerkLines[clerk]->transactionLock);
	}

	printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
			clerkLines[clerk]->name);
	clerkLines[clerk]->transactionLock->Release();
}

void beClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Picture Clerk
	//-------------------------------------------------------------------
	while (clerkLines[clerkIndex]->type == Clerk::PIC) {
		picappLineLock.Acquire();
		printf("%s acquired %s.\n", clerkLines[clerkIndex]->name,
				picappLineLock.getName());

		if (clerkLines[clerkIndex]->lineCount > 0) {
			printf("%s is available and there is someone in line, signaling.\n",
					clerkLines[clerkIndex]->name);
			clerkLines[clerkIndex]->lineCV->Signal(&picappLineLock);
			clerkLines[clerkIndex]->state = Clerk::BUSY;
		}
		else {
			printf("No one in line for %s -- available.\n", clerkLines[clerkIndex]->name);
			clerkLines[clerkIndex]->state = Clerk::AVAILABLE;
		}

		clerkLines[clerkIndex]->transactionLock->Acquire();
		printf("%s acquired transaction lock %s.\n",
				clerkLines[clerkIndex]->name,
				clerkLines[clerkIndex]->transactionLock->getName());

		printf("%s released %s.\n", clerkLines[clerkIndex]->name,
				picappLineLock.getName());
		picappLineLock.Release();

		// wait for Customer data
		printf("%s waiting on transaction.\n", clerkLines[clerkIndex]->name);
		clerkLines[clerkIndex]->transactionCV->Wait(
				clerkLines[clerkIndex]->transactionLock);

		// Doing job, customer waiting, signal when done
		while (!clerkLines[clerkIndex]->customer->picDone) {
			printf("%s taking the picture for %s.\n",
					clerkLines[clerkIndex]->name,
					clerkLines[clerkIndex]->customer->name);
			clerkLines[clerkIndex]->transactionCV->Signal(
					clerkLines[clerkIndex]->transactionLock);
			// Waiting for customer to accept photo
			printf("%s is waiting for %s's reaction.\n",
					clerkLines[clerkIndex]->name,
					clerkLines[clerkIndex]->customer->name);
			clerkLines[clerkIndex]->transactionCV->Wait(
					clerkLines[clerkIndex]->transactionLock);
		}
		printf("%s's picture is being filed.\n", clerkLines[clerkIndex]->customer->name);
		// Yield for a bit
		for(int i=0; i < randomYield(); ++i) {
			currentThread->Yield();
		}
		clerkLines[clerkIndex]->transactionCV->Signal(
							clerkLines[clerkIndex]->transactionLock);
		printf("%s's picture is now filed.\n", clerkLines[clerkIndex]->customer->name);
		printf("%s is done at %s. ", clerkLines[clerkIndex]->customer->name,
				clerkLines[clerkIndex]->name);
//		printf("%s is becoming available.\n", clerkLines[clerkIndex]->name);
//		clerkLines[clerkIndex]->state = Clerk::AVAILABLE;

		printf("%s released transaction lock %s.\n",
				clerkLines[clerkIndex]->name,
				clerkLines[clerkIndex]->transactionLock->getName());
		clerkLines[clerkIndex]->transactionLock->Release();
	}
	//-------------------------------------------------------------------
	// Application Clerk
	//-------------------------------------------------------------------
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

		picappLineLock.Acquire();
		printf("%s acquired %s.\n", currentThread->getName(),
				picappLineLock.getName());

		// can choose either pic or app line
		if (!customers[customerIndex]->picDone
				&& !customers[customerIndex]->appDone) {
			for (int i = 0; i < ARRAY_SIZE(clerkLines); ++i) {
				if (clerkLines[i]->lineCount < lineSize
						&& clerkLines[i]->state != Clerk::BREAK) {
					myLine = i;
					lineSize = clerkLines[i]->lineCount;
				}
			}
		}
		else if (!customers[customerIndex]->picDone) { // app already done, has to choose pic line
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
			clerkLines[myLine]->lineCV->Wait(&picappLineLock);
			printf("%s done waiting for %s.\n", customers[customerIndex]->name,
					clerkLines[myLine]->name);
			clerkLines[myLine]->lineCount--;
		}

		// Clerk is now available, current customer can approach the clerk.
//		ASSERT(clerkLines[myLine]->state != Clerk::BUSY);
		clerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

//		printf("%s acquired lock %s.\n", currentThread->getName(), clerkLines[myLine]->lock->getName());
//		clerkLines[myLine]->lock->Acquire();

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
//	clerkLines[1] = new Clerk("App Clerk 1", Clerk::APP);

	for (i = 0; i < 1; ++i) {
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

