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
	bool passportDone;

	Customer(char * n) {
		name = n;
		picDone = false;
		appDone = false;
		passportDone = false;
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
Lock passportLineLock("Passpork Line Lock");

const int NUM_CUSTOMERS = 1;

Clerk * picAppClerkLines[2];
Clerk * passportClerkLines[1];

Customer * customers[NUM_CUSTOMERS];

void picAppClerkTransaction(int customer, int clerk) {
	// Set the clerk's current customer
	picAppClerkLines[clerk]->transactionLock->Acquire();
	picAppClerkLines[clerk]->customer = customers[customer];
	printf("%s acquired lock %s and set himself as the customer.\n",
			customers[customer]->name,
			picAppClerkLines[clerk]->transactionLock->getName());

	// Do the picture process
	if (picAppClerkLines[clerk]->type == Clerk::PIC) {
		// take picture
		printf("%s approaches clerk %s, signaling to begin interaction.\n",
				currentThread->getName(), picAppClerkLines[clerk]->name);
		picAppClerkLines[clerk]->transactionCV->Signal(
				picAppClerkLines[clerk]->transactionLock);

		// Wait to be shown picture
		// Take picture until he likes it
		while (!customers[customer]->picDone) {
			printf("%s waiting for clerk %s to take the picture.\n",
					currentThread->getName(), picAppClerkLines[clerk]->name);
			picAppClerkLines[clerk]->transactionCV->Wait(
					picAppClerkLines[clerk]->transactionLock);

			if ((rand() % 10) % 2 == 0) {
				printf("%s doesn't like the photo, signaling to clerk %s.\n",
						currentThread->getName(),
						picAppClerkLines[clerk]->name);
				picAppClerkLines[clerk]->transactionCV->Signal(
						picAppClerkLines[clerk]->transactionLock);
			}
			else {
				printf("%s likes the photo, signaling to clerk %s.\n",
						currentThread->getName(),
						picAppClerkLines[clerk]->name);
				customers[customer]->picDone = true;
				picAppClerkLines[clerk]->transactionCV->Signal(
						picAppClerkLines[clerk]->transactionLock);
			}
		}
		// customer liked the photo and is leaving
		printf("%s is waiting for the photo to be filed.\n",
				currentThread->getName());
		picAppClerkLines[clerk]->transactionCV->Wait(
				picAppClerkLines[clerk]->transactionLock);
	}
	else if (picAppClerkLines[clerk]->type == Clerk::APP) {
		// take picture
		printf("%s approaches clerk %s, submitting SSN and application.\n",
				currentThread->getName(), picAppClerkLines[clerk]->name);
		picAppClerkLines[clerk]->transactionCV->Signal(
				picAppClerkLines[clerk]->transactionLock);

		printf("%s waiting for clerk %s to file the application.\n",
				currentThread->getName(), picAppClerkLines[clerk]->name);
		picAppClerkLines[clerk]->transactionCV->Wait(
				picAppClerkLines[clerk]->transactionLock);
	}

	printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
			picAppClerkLines[clerk]->name);
	picAppClerkLines[clerk]->transactionLock->Release();
}

void passportClerkTransaction(int customer, int clerk) {
	passportClerkLines[clerk]->transactionLock->Acquire();
	passportClerkLines[clerk]->customer = customers[customer];
	printf("%s acquired lock %s and set himself as the customer.\n",
			customers[customer]->name,
			passportClerkLines[clerk]->transactionLock->getName());

	printf(
			"%s approaches clerk %s to check if picture and application are filed.\n",
			customers[customer]->name, passportClerkLines[clerk]->name);
	passportClerkLines[clerk]->transactionCV->Signal(
			passportClerkLines[clerk]->transactionLock);

	printf(
			"%s waiting for clerk %s to certify if picture and application are filed.\n",
			customers[customer]->name, passportClerkLines[clerk]->name);
	passportClerkLines[clerk]->transactionCV->Wait(
			passportClerkLines[clerk]->transactionLock);

	ASSERT(customers[customer]->appDone && customers[customer]->picDone);
	printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
			passportClerkLines[clerk]->name);
	passportClerkLines[clerk]->transactionLock->Release();
}

void bePicAppClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Picture Clerk
	//-------------------------------------------------------------------
	while (picAppClerkLines[clerkIndex]->type == Clerk::PIC) {
		picappLineLock.Acquire();
		printf("%s acquired %s.\n", picAppClerkLines[clerkIndex]->name,
				picappLineLock.getName());

		if (picAppClerkLines[clerkIndex]->lineCount > 0) {
			printf("%s is available and there is someone in line, signaling.\n",
					picAppClerkLines[clerkIndex]->name);
			picAppClerkLines[clerkIndex]->lineCV->Signal(&picappLineLock);
			picAppClerkLines[clerkIndex]->state = Clerk::BUSY;
		}
		else {
			printf("No one in line for %s -- available.\n",
					picAppClerkLines[clerkIndex]->name);
			picAppClerkLines[clerkIndex]->state = Clerk::AVAILABLE;
		}

		picAppClerkLines[clerkIndex]->transactionLock->Acquire();
		printf("%s acquired transaction lock %s.\n",
				picAppClerkLines[clerkIndex]->name,
				picAppClerkLines[clerkIndex]->transactionLock->getName());

		printf("%s released %s.\n", picAppClerkLines[clerkIndex]->name,
				picappLineLock.getName());
		picappLineLock.Release();

		// wait for Customer data
		printf("%s waiting on transaction.\n",
				picAppClerkLines[clerkIndex]->name);
		picAppClerkLines[clerkIndex]->transactionCV->Wait(
				picAppClerkLines[clerkIndex]->transactionLock);

		// Doing job, customer waiting, signal when done
		while (!picAppClerkLines[clerkIndex]->customer->picDone) {
			printf("%s taking the picture for %s.\n",
					picAppClerkLines[clerkIndex]->name,
					picAppClerkLines[clerkIndex]->customer->name);
			picAppClerkLines[clerkIndex]->transactionCV->Signal(
					picAppClerkLines[clerkIndex]->transactionLock);
			// Waiting for customer to accept photo
			printf("%s is waiting for %s's reaction.\n",
					picAppClerkLines[clerkIndex]->name,
					picAppClerkLines[clerkIndex]->customer->name);
			picAppClerkLines[clerkIndex]->transactionCV->Wait(
					picAppClerkLines[clerkIndex]->transactionLock);
		}
		printf("%s's picture is being filed.\n",
				picAppClerkLines[clerkIndex]->customer->name);
		// Yield for a bit

		for (int i = 0; i < rand() % 80 + 20; ++i) {
			currentThread->Yield();
		}
		picAppClerkLines[clerkIndex]->transactionCV->Signal(
				picAppClerkLines[clerkIndex]->transactionLock);
		printf("%s's picture is now filed.\n",
				picAppClerkLines[clerkIndex]->customer->name);
		printf("%s is done at %s. ",
				picAppClerkLines[clerkIndex]->customer->name,
				picAppClerkLines[clerkIndex]->name);

		printf("%s released transaction lock %s.\n",
				picAppClerkLines[clerkIndex]->name,
				picAppClerkLines[clerkIndex]->transactionLock->getName());
		picAppClerkLines[clerkIndex]->transactionLock->Release();
	}
	//-------------------------------------------------------------------
	// Application Clerk
	//-------------------------------------------------------------------
	while (picAppClerkLines[clerkIndex]->type == Clerk::APP) {
		picappLineLock.Acquire();
		printf("%s acquired %s.\n", picAppClerkLines[clerkIndex]->name,
				picappLineLock.getName());

		if (picAppClerkLines[clerkIndex]->lineCount > 0) {
			printf("%s is available and there is someone in line, signaling.\n",
					picAppClerkLines[clerkIndex]->name);
			picAppClerkLines[clerkIndex]->lineCV->Signal(&picappLineLock);
			picAppClerkLines[clerkIndex]->state = Clerk::BUSY;
		}
		else {
			printf("No one in line for %s -- available.\n",
					picAppClerkLines[clerkIndex]->name);
			picAppClerkLines[clerkIndex]->state = Clerk::AVAILABLE;
		}

		picAppClerkLines[clerkIndex]->transactionLock->Acquire();
		printf("%s acquired transaction lock %s.\n",
				picAppClerkLines[clerkIndex]->name,
				picAppClerkLines[clerkIndex]->transactionLock->getName());

		printf("%s released %s.\n", picAppClerkLines[clerkIndex]->name,
				picappLineLock.getName());
		picappLineLock.Release();

		// wait for Customer data
		printf("%s waiting on transaction.\n",
				picAppClerkLines[clerkIndex]->name);
		picAppClerkLines[clerkIndex]->transactionCV->Wait(
				picAppClerkLines[clerkIndex]->transactionLock);

		// Doing job, customer waiting, signal when done
		printf("%s filing application for %s.\n",
				picAppClerkLines[clerkIndex]->name,
				picAppClerkLines[clerkIndex]->customer->name);
		// Yield for a bit
		for (int i = 0; i < rand() % 80 + 20; ++i) {
			currentThread->Yield();
		}
		// set application as complete
		picAppClerkLines[clerkIndex]->customer->appDone = true;
		picAppClerkLines[clerkIndex]->transactionCV->Signal(
				picAppClerkLines[clerkIndex]->transactionLock);
		printf("%s's application is now filed.\n",
				picAppClerkLines[clerkIndex]->customer->name);
		printf("%s is done at %s. ",
				picAppClerkLines[clerkIndex]->customer->name,
				picAppClerkLines[clerkIndex]->name);

		printf("%s released transaction lock %s.\n",
				picAppClerkLines[clerkIndex]->name,
				picAppClerkLines[clerkIndex]->transactionLock->getName());
		picAppClerkLines[clerkIndex]->transactionLock->Release();
	}
}

void bePassportClerk(int clerkIndex) {
	passportLineLock.Acquire();
	printf("%s acquired %s.\n", passportClerkLines[clerkIndex]->name,
			passportLineLock.getName());

	if (passportClerkLines[clerkIndex]->lineCount > 0) {
		printf("%s is available and there is someone in line, signaling.\n",
				passportClerkLines[clerkIndex]->name);
		passportClerkLines[clerkIndex]->lineCV->Signal(&passportLineLock);
		passportClerkLines[clerkIndex]->state = Clerk::BUSY;
	}
	else {
		printf("No one in line for %s -- available.\n",
				passportClerkLines[clerkIndex]->name);
		passportClerkLines[clerkIndex]->state = Clerk::AVAILABLE;
	}

	passportClerkLines[clerkIndex]->transactionLock->Acquire();
	printf("%s acquired transaction lock %s.\n",
			passportClerkLines[clerkIndex]->name,
			passportClerkLines[clerkIndex]->transactionLock->getName());

	printf("%s released %s.\n", passportClerkLines[clerkIndex]->name,
			passportLineLock.getName());
	passportLineLock.Release();

	// wait for Customer data
	printf("%s waiting on transaction.\n", passportClerkLines[clerkIndex]->name);
	passportClerkLines[clerkIndex]->transactionCV->Wait(
			passportClerkLines[clerkIndex]->transactionLock);

	// Doing job, customer waiting, signal when done
	printf("%s certifying complete application and picture for %s.\n",
			passportClerkLines[clerkIndex]->name,
			passportClerkLines[clerkIndex]->customer->name);

	// Yield for a bit
	for (int i = 0; i < rand() % 900 + 100; ++i) {
		currentThread->Yield();
	}
	// set application as complete
	ASSERT(passportClerkLines[clerkIndex]->customer->appDone && passportClerkLines[clerkIndex]->customer->picDone);
	passportClerkLines[clerkIndex]->transactionCV->Signal(
			passportClerkLines[clerkIndex]->transactionLock);
	printf("%s's application and picture are approved.\n",
			passportClerkLines[clerkIndex]->customer->name);
	printf("%s is done at %s. ", passportClerkLines[clerkIndex]->customer->name,
			passportClerkLines[clerkIndex]->name);

	printf("%s released transaction lock %s.\n",
			passportClerkLines[clerkIndex]->name,
			passportClerkLines[clerkIndex]->transactionLock->getName());
	passportClerkLines[clerkIndex]->transactionLock->Release();
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
			|| !customers[customerIndex]->appDone) {
		myLine = -1;
		lineSize = 1000;

		picappLineLock.Acquire();
		printf("%s acquired %s.\n", currentThread->getName(),
				picappLineLock.getName());

		// can choose either pic or app line
		if (!customers[customerIndex]->picDone
				&& !customers[customerIndex]->appDone) {
			for (int i = 0; i < ARRAY_SIZE(picAppClerkLines); ++i) {
				if (picAppClerkLines[i]->lineCount < lineSize) {
					myLine = i;
					lineSize = picAppClerkLines[i]->lineCount;
				}
			}
		}
		else if (!customers[customerIndex]->picDone) { // app already done, has to choose pic line
			for (int i = 0; i < ARRAY_SIZE(picAppClerkLines); ++i) {
				if (picAppClerkLines[i]->type == Clerk::PIC
						&& picAppClerkLines[i]->lineCount < lineSize) {
					myLine = i;
					lineSize = picAppClerkLines[i]->lineCount;
				}
			}
		}
		else if (!customers[customerIndex]->appDone) { // has to choose app line
			for (int i = 0; i < ARRAY_SIZE(picAppClerkLines); ++i) {
				if (picAppClerkLines[i]->type == Clerk::APP
						&& picAppClerkLines[i]->lineCount < lineSize) {
					myLine = i;
					lineSize = picAppClerkLines[i]->lineCount;
				}
			}
		}

		printf("%s chose %s.\n", customers[customerIndex]->name,
				picAppClerkLines[myLine]->name);
		// Customer must wait for clerk to become available.

		if (picAppClerkLines[myLine]->state != Clerk::AVAILABLE) {
			picAppClerkLines[myLine]->lineCount++;
			printf("%s waiting for %s.\n", customers[customerIndex]->name,
					picAppClerkLines[myLine]->name);
			picAppClerkLines[myLine]->lineCV->Wait(&picappLineLock);
			printf("%s done waiting for %s.\n", customers[customerIndex]->name,
					picAppClerkLines[myLine]->name);
			picAppClerkLines[myLine]->lineCount--;
		}

		// Clerk is now available, current customer can approach the clerk.
//		ASSERT(clerkLines[myLine]->state != Clerk::BUSY);
		picAppClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

//		printf("%s acquired lock %s.\n", currentThread->getName(), clerkLines[myLine]->lock->getName());
//		clerkLines[myLine]->lock->Acquire();

		printf("%s releasing %s\n", customers[customerIndex]->name,
				picappLineLock.getName());
		picappLineLock.Release();
		// interaction begins
		picAppClerkTransaction(customerIndex, myLine);
	}

	printf("%s finished picture and application tasks.\n",
			currentThread->getName());
	//-------------------------------------------------------------------
	// Step 2: Going to passport clerk
	//-------------------------------------------------------------------

	while (customers[customerIndex]->appDone
			&& customers[customerIndex]->picDone
			&& !customers[customerIndex]->passportDone) {
		// choose shortest passport clerk line
		passportLineLock.Acquire();
		myLine = -1;
		lineSize = 1000;

		for (int i = 0; i < ARRAY_SIZE(passportClerkLines); ++i) {
			if (passportClerkLines[i]->lineCount < lineSize) {
				myLine = i;
				lineSize = passportClerkLines[i]->lineCount;
			}
		}

		printf("%s chose %s.\n", customers[customerIndex]->name,
				passportClerkLines[myLine]->name);
		// Customer must wait for clerk to become available.

		if (passportClerkLines[myLine]->state != Clerk::AVAILABLE) {
			passportClerkLines[myLine]->lineCount++;
			printf("%s waiting for %s.\n", customers[customerIndex]->name,
					passportClerkLines[myLine]->name);
			passportClerkLines[myLine]->lineCV->Wait(&passportLineLock);
			printf("%s done waiting for %s.\n", customers[customerIndex]->name,
					picAppClerkLines[myLine]->name);
			passportClerkLines[myLine]->lineCount--;
		}

		// Clerk is now available, current customer can approach the clerk.
		passportClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

		printf("%s releasing %s\n", customers[customerIndex]->name,
				passportLineLock.getName());
		passportLineLock.Release();
		// interaction begins
		passportClerkTransaction(customerIndex, myLine);
	}
}

void PassportOffice() {
	Thread * t;
	char * name;
	int i;

	srand(time(NULL));

	passportClerkLines[0] = new Clerk("Passport Clerk 0", Clerk::PP);

	picAppClerkLines[0] = new Clerk("Pic Clerk 0", Clerk::PIC);
	picAppClerkLines[1] = new Clerk("App Clerk 1", Clerk::APP);

	for (i = 0; i < 1; ++i) {
		name = passportClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePassportClerk, i);
	}

	for (i = 0; i < 2; ++i) {
		name = picAppClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePicAppClerk, i);
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

