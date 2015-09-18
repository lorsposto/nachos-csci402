/*
 * passport_office.cc
 *
 *  Created on: Sep 11, 2015
 *  Authors: Erika Johnson, Heather Persson, Lorraine Sposto
 *
 */
#include "utility.h"
#include "system.h"
#include "synch.h"
#include "timer.h"
#include <time.h>
#include <stdlib.h>

#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

void broadcastMoney(int x);

struct Customer {
	char * name;
	bool picDone;
	bool appDone;
	bool certified;
	bool gotPassport;
	int money;

	Customer(char * n) {
		name = n;
		picDone = false;
		appDone = false;
		certified = false;
		gotPassport = false;
		switch (rand() % 4) {
		case 0:
			money = 100;
			break;
		case 1:
			money = 600;
			break;
		case 2:
			money = 1100;
			break;
		case 3:
			money = 1600;
			break;
		}
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
	int bribeLineCount;
	int regularLineCount;
	clerkType type;
	clerkState state;
	Condition * regularLineCV;
	Condition * bribeLineCV;
	Condition * transactionCV;
	Lock * transactionLock;

	Condition * breakCV;
	Lock * breakLock;

	Customer * customer;
	int money;

	Clerk() {
		name = NULL;
		regularLineCount = 0;
		bribeLineCount = 0;
		state = AVAILABLE;
		type = APP;
		regularLineCV = new Condition(name);
		bribeLineCV = new Condition(name);
		transactionCV = new Condition(name);
		transactionLock = new Lock(name);
		customer = NULL;
		money = 0;

		breakCV = new Condition(name);
		breakLock = new Lock(name);
	}

	Clerk(char * n, clerkType t) {
		name = n;
		regularLineCount = 0;
		bribeLineCount = 0;
		state = AVAILABLE;
		type = t;
		regularLineCV = new Condition(name);
		bribeLineCV = new Condition(name);
		transactionLock = new Lock(name);
		transactionCV = new Condition(name);
		customer = NULL;
		money = 0;

		breakCV = new Condition(name);
		breakLock = new Lock(name);
	}
};

class Manager {
public:
	char * name;
	Timer * timer;

	Manager(char * n) {
		name = n;
//		timer = new Timer((VoidFunctionPtr) broadcastMoney, 50, true);
	}
};

struct Cashier {
public:
	enum cashierState {
		AVAILABLE, BUSY, BREAK
	};
	char * name;
	int lineCount;
	int money;
	cashierState state;
	Condition * lineCV;
	Condition * transactionCV;
	Lock * transactionLock;

	Condition * breakCV;
	Lock * breakLock;

	Customer * customer;

	Cashier() {
		name = NULL;
		lineCount = 0;
		state = AVAILABLE;
		lineCV = new Condition(name);
		transactionCV = new Condition(name);
		transactionLock = new Lock(name);
		customer = NULL;

		breakCV = new Condition(name);
		breakLock = new Lock(name);
	}

	Cashier(char * n) {
		name = n;
		lineCount = 0;
		state = AVAILABLE;
		lineCV = new Condition(name);
		transactionLock = new Lock(name);
		transactionCV = new Condition(name);
		customer = NULL;

		breakCV = new Condition(name);
		breakLock = new Lock(name);
	}
};

// Customer's first step, either app or pic clerk
Lock picLineLock("Pic Line Lock");
Lock appLineLock("App Line Lock");
Lock passportLineLock("Passport Line Lock");
Lock cashierLineLock("Cashier Line Lock");

const int NUM_CUSTOMERS = 1;

Clerk * picClerkLines[1];
Clerk * appClerkLines[1];
Clerk * passportClerkLines[1];
Cashier * cashierLines[1];

Manager * managers[1];

Customer * customers[NUM_CUSTOMERS];

void broadcastMoney(int x) {
	// TODO add the rest of the locks
	int total = 0;
	// Read all of the clerks/cashiers and print their sums.
	for (unsigned int i = 0; i < ARRAY_SIZE(picClerkLines); ++i) {
		picClerkLines[i]->transactionLock->Acquire();
		total += picClerkLines[i]->money;
		printf("Manager announces that %s has $%i\n", picClerkLines[i]->name,
				picClerkLines[i]->money);
		picClerkLines[i]->transactionLock->Release();
	}
	for (unsigned int i = 0; i < ARRAY_SIZE(appClerkLines); ++i) {
		appClerkLines[i]->transactionLock->Acquire();
		total += appClerkLines[i]->money;
		printf("Manager announces that %s has $%i\n", appClerkLines[i]->name,
				appClerkLines[i]->money);
		appClerkLines[i]->transactionLock->Release();
	}
	for (unsigned int i = 0; i < ARRAY_SIZE(passportClerkLines); ++i) {
		passportClerkLines[i]->transactionLock->Acquire();
		total += passportClerkLines[i]->money;
		printf("Manager announces that %s has $%i\n",
				passportClerkLines[i]->name, passportClerkLines[i]->money);
		passportClerkLines[i]->transactionLock->Release();
	}
	for (unsigned int i = 0; i < ARRAY_SIZE(cashierLines); ++i) {
		cashierLines[i]->transactionLock->Acquire();
		total += cashierLines[i]->money;
		printf("Manager announces that %s has $%i\n", cashierLines[i]->name,
				cashierLines[i]->money);
		cashierLines[i]->transactionLock->Release();
	}
	printf("Manager announces that the total is $%i\n", total);
}

void beManager(int index) {
	while (true) {
		// TODO add other clerk cashiers blah
		for (unsigned int i = 0; i < ARRAY_SIZE(picClerkLines); ++i) {
			picClerkLines[i]->transactionLock->Acquire();
			if (picClerkLines[i]->state == Clerk::BREAK
					&& (picClerkLines[i]->bribeLineCount
							+ picClerkLines[i]->regularLineCount) > 0) {
				picClerkLines[i]->state = Clerk::AVAILABLE;
				printf("%s is waking up %s.\n", managers[index]->name,
						picClerkLines[i]->name);
				picClerkLines[i]->breakCV->Signal(picClerkLines[i]->breakLock);
			}

			// Yield a bit
			for (unsigned int y = 0; y < 500; ++y) {
				currentThread->Yield();
			}
			picClerkLines[i]->transactionLock->Release();
		}
		for (unsigned int i = 0; i < ARRAY_SIZE(appClerkLines); ++i) {
			appClerkLines[i]->transactionLock->Acquire();
			if (appClerkLines[i]->state == Clerk::BREAK
					&& (appClerkLines[i]->bribeLineCount
							+ appClerkLines[i]->regularLineCount) > 0) {
				appClerkLines[i]->state = Clerk::AVAILABLE;
				printf("%s is waking up %s.\n", managers[index]->name,
						appClerkLines[i]->name);
				appClerkLines[i]->breakCV->Signal(appClerkLines[i]->breakLock);
			}

			// Yield a bit
			for (unsigned int y = 0; y < 500; ++y) {
				currentThread->Yield();
			}
			appClerkLines[i]->transactionLock->Release();
		}
		for (unsigned int i = 0; i < ARRAY_SIZE(passportClerkLines); ++i) {
			passportClerkLines[i]->transactionLock->Acquire();
			if (passportClerkLines[i]->state == Clerk::BREAK
					&& (passportClerkLines[i]->bribeLineCount
							+ passportClerkLines[i]->regularLineCount) > 0) {
				passportClerkLines[i]->state = Clerk::AVAILABLE;
				printf("%s is waking up %s.\n", managers[index]->name,
						passportClerkLines[i]->name);
				passportClerkLines[i]->breakCV->Signal(
						passportClerkLines[i]->breakLock);
			}
			// Yield a bit
			for (unsigned int y = 0; y < 500; ++y) {
				currentThread->Yield();
			}
			passportClerkLines[i]->transactionLock->Release();
		}
		for (unsigned int i = 0; i < ARRAY_SIZE(cashierLines); ++i) {
			cashierLines[i]->transactionLock->Acquire();
			if (cashierLines[i]->state == Cashier::BREAK
					&& cashierLines[i]->lineCount > 0) {
				cashierLines[i]->state = Cashier::AVAILABLE;
				printf("%s is waking up %s.\n", managers[index]->name,
						cashierLines[i]->name);
				cashierLines[i]->breakCV->Signal(cashierLines[i]->breakLock);
			}
			// Yield a bit
			for (unsigned int y = 0; y < 500; ++y) {
				currentThread->Yield();
			}
			cashierLines[i]->transactionLock->Release();
		}
	}
}

void picClerkTransaction(int customer, int clerk) {
	// Set the clerk's current customer
	picClerkLines[clerk]->transactionLock->Acquire();
	picClerkLines[clerk]->customer = customers[customer];
	printf("%s acquired lock %s and set himself as the customer.\n",
			customers[customer]->name,
			picClerkLines[clerk]->transactionLock->getName());

	// take picture
	printf("%s approaches clerk %s, signaling to begin interaction.\n",
			currentThread->getName(), picClerkLines[clerk]->name);
	picClerkLines[clerk]->transactionCV->Signal(
			picClerkLines[clerk]->transactionLock);

	// Wait to be shown picture
	// Take picture until he likes it
	while (!customers[customer]->picDone) {
		printf("%s waiting for clerk %s to take the picture.\n",
				currentThread->getName(), picClerkLines[clerk]->name);
		picClerkLines[clerk]->transactionCV->Wait(
				picClerkLines[clerk]->transactionLock);

		if ((rand() % 10) % 2 == 0) {
			printf("%s doesn't like the photo, signaling to clerk %s.\n",
					currentThread->getName(), picClerkLines[clerk]->name);
			picClerkLines[clerk]->transactionCV->Signal(
					picClerkLines[clerk]->transactionLock);
		}
		else {
			printf("%s likes the photo, signaling to clerk %s.\n",
					currentThread->getName(), picClerkLines[clerk]->name);
			customers[customer]->picDone = true;
			picClerkLines[clerk]->transactionCV->Signal(
					picClerkLines[clerk]->transactionLock);
		}
	}
	// customer liked the photo and is leaving
	printf("%s is waiting for the photo to be filed.\n",
			currentThread->getName());
	picClerkLines[clerk]->transactionCV->Wait(
			picClerkLines[clerk]->transactionLock);

	printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
			picClerkLines[clerk]->name);
	picClerkLines[clerk]->transactionLock->Release();
}

void appClerkTransaction(int customer, int clerk) {
	// Set the clerk's current customer
	appClerkLines[clerk]->transactionLock->Acquire();
	appClerkLines[clerk]->customer = customers[customer];
	printf("%s acquired lock %s and set himself as the customer.\n",
			customers[customer]->name,
			appClerkLines[clerk]->transactionLock->getName());

	// take picture
	printf("%s approaches clerk %s, submitting SSN and application.\n",
			currentThread->getName(), appClerkLines[clerk]->name);
	appClerkLines[clerk]->transactionCV->Signal(
			appClerkLines[clerk]->transactionLock);

	printf("%s waiting for clerk %s to file the application.\n",
			currentThread->getName(), appClerkLines[clerk]->name);
	appClerkLines[clerk]->transactionCV->Wait(
			appClerkLines[clerk]->transactionLock);

	printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
			appClerkLines[clerk]->name);
	appClerkLines[clerk]->transactionLock->Release();
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

void cashierTransaction(int customer, int cashier) {
	// Set the cashier's current customer
	cashierLines[cashier]->transactionLock->Acquire();
	cashierLines[cashier]->customer = customers[customer];
	printf("%s acquired lock %s and set himself as the customer.\n",
			customers[customer]->name,
			cashierLines[cashier]->transactionLock->getName());

	printf("%s approaches cashier %s to give him money.\n",
			currentThread->getName(), cashierLines[cashier]->name);
	cashierLines[cashier]->transactionCV->Signal(
			cashierLines[cashier]->transactionLock);

	printf("%s waiting for cashier %s to accept money and provide passport.\n",
			currentThread->getName(), cashierLines[cashier]->name);
	cashierLines[cashier]->transactionCV->Wait(
			cashierLines[cashier]->transactionLock);

	printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
			cashierLines[cashier]->name);
	cashierLines[cashier]->transactionLock->Release();
}

void bePicClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Picture Clerk
	//-------------------------------------------------------------------
	while (true) {
		while (picClerkLines[clerkIndex]->state != Clerk::BREAK) {
			picLineLock.Acquire();
			printf("%s acquired %s.\n", picClerkLines[clerkIndex]->name,
					picLineLock.getName());

			if (picClerkLines[clerkIndex]->bribeLineCount > 0) {
				printf(
						"%s is available and there is someone in the bribe line, signaling.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->bribeLineCV->Signal(&picLineLock);
				picClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else if (picClerkLines[clerkIndex]->regularLineCount > 0) {
				printf(
						"%s is available and there is someone in the regular line, signaling.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->regularLineCV->Signal(&picLineLock);
				picClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else {
				printf("No one in line for %s.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->state = Clerk::BREAK;
				printf("%s is now on break.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->breakCV->Wait(
						picClerkLines[clerkIndex]->breakLock);
				break;
			}

			picClerkLines[clerkIndex]->transactionLock->Acquire();
			printf("%s acquired transaction lock %s.\n",
					picClerkLines[clerkIndex]->name,
					picClerkLines[clerkIndex]->transactionLock->getName());

			printf("%s released %s.\n", picClerkLines[clerkIndex]->name,
					picLineLock.getName());
			picLineLock.Release();

			// wait for Customer data
			printf("%s waiting on transaction.\n",
					picClerkLines[clerkIndex]->name);
			picClerkLines[clerkIndex]->transactionCV->Wait(
					picClerkLines[clerkIndex]->transactionLock);

			// Doing job, customer waiting, signal when done
			while (!picClerkLines[clerkIndex]->customer->picDone) {
				printf("%s taking the picture for %s.\n",
						picClerkLines[clerkIndex]->name,
						picClerkLines[clerkIndex]->customer->name);
				picClerkLines[clerkIndex]->transactionCV->Signal(
						picClerkLines[clerkIndex]->transactionLock);
				// Waiting for customer to accept photo
				printf("%s is waiting for %s's reaction.\n",
						picClerkLines[clerkIndex]->name,
						picClerkLines[clerkIndex]->customer->name);
				picClerkLines[clerkIndex]->transactionCV->Wait(
						picClerkLines[clerkIndex]->transactionLock);
			}
			printf("%s's picture is being filed.\n",
					picClerkLines[clerkIndex]->customer->name);

			picClerkLines[clerkIndex]->transactionCV->Signal(
					picClerkLines[clerkIndex]->transactionLock);
			printf("%s's picture is now filed.\n",
					picClerkLines[clerkIndex]->customer->name);
			printf("%s is done at %s. ",
					picClerkLines[clerkIndex]->customer->name,
					picClerkLines[clerkIndex]->name);

			printf("%s released transaction lock %s.\n",
					picClerkLines[clerkIndex]->name,
					picClerkLines[clerkIndex]->transactionLock->getName());
			picClerkLines[clerkIndex]->transactionLock->Release();
		}
	}
}

void beAppClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Application Clerk
	//-------------------------------------------------------------------
	while (true) {
		while (appClerkLines[clerkIndex]->state != Clerk::BREAK) {
			appLineLock.Acquire();
			printf("%s acquired %s.\n", appClerkLines[clerkIndex]->name,
					appLineLock.getName());

			if (appClerkLines[clerkIndex]->bribeLineCount > 0) {
				printf(
						"%s is available and there is someone in the bribe line, signaling.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->bribeLineCV->Signal(&appLineLock);
				appClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else if (appClerkLines[clerkIndex]->regularLineCount > 0) {
				printf(
						"%s is available and there is someone in regular line, signaling.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->regularLineCV->Signal(&appLineLock);
				appClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else {
				printf("No one in line for %s.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->state = Clerk::BREAK;

				printf("%s is now on break.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->breakCV->Wait(
						appClerkLines[clerkIndex]->breakLock);
				break;
			}

			appClerkLines[clerkIndex]->transactionLock->Acquire();
			printf("%s acquired transaction lock %s.\n",
					appClerkLines[clerkIndex]->name,
					appClerkLines[clerkIndex]->transactionLock->getName());

			printf("%s released %s.\n", appClerkLines[clerkIndex]->name,
					appLineLock.getName());
			appLineLock.Release();

			// wait for Customer data
			printf("%s waiting on transaction.\n",
					appClerkLines[clerkIndex]->name);
			appClerkLines[clerkIndex]->transactionCV->Wait(
					appClerkLines[clerkIndex]->transactionLock);

			// Doing job, customer waiting, signal when done
			printf("%s filing application for %s.\n",
					appClerkLines[clerkIndex]->name,
					appClerkLines[clerkIndex]->customer->name);
			// Yield for a bit
			for (int i = 0; i < rand() % 80 + 20; ++i) {
				currentThread->Yield();
			}
			// set application as complete
			appClerkLines[clerkIndex]->customer->appDone = true;
			appClerkLines[clerkIndex]->transactionCV->Signal(
					appClerkLines[clerkIndex]->transactionLock);
			printf("%s's application is now filed.\n",
					appClerkLines[clerkIndex]->customer->name);
			printf("%s is done at %s. ",
					appClerkLines[clerkIndex]->customer->name,
					appClerkLines[clerkIndex]->name);

			printf("%s released transaction lock %s.\n",
					appClerkLines[clerkIndex]->name,
					appClerkLines[clerkIndex]->transactionLock->getName());
			appClerkLines[clerkIndex]->transactionLock->Release();
		}
	}
}

void bePassportClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Passport Clerk
	//-------------------------------------------------------------------
	while (true) {
		while (passportClerkLines[clerkIndex]->state != Clerk::BREAK) {
			passportLineLock.Acquire();
			printf("%s acquired %s.\n", passportClerkLines[clerkIndex]->name,
					passportLineLock.getName());
			if (passportClerkLines[clerkIndex]->bribeLineCount > 0) {
				printf(
						"%s is available and there is someone in bribe line, signaling.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->bribeLineCV->Signal(
						&passportLineLock);
				passportClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else if (passportClerkLines[clerkIndex]->regularLineCount > 0) {
				printf(
						"%s is available and there is someone in regular line, signaling.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->regularLineCV->Signal(
						&passportLineLock);
				passportClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else {
				printf("No one in line for %s.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->state = Clerk::BREAK;
				printf("%s is now on break.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->breakCV->Wait(
						passportClerkLines[clerkIndex]->breakLock);
				break;
			}

			passportClerkLines[clerkIndex]->transactionLock->Acquire();
			printf("%s acquired transaction lock %s.\n",
					passportClerkLines[clerkIndex]->name,
					passportClerkLines[clerkIndex]->transactionLock->getName());

			printf("%s released %s.\n", passportClerkLines[clerkIndex]->name,
					passportLineLock.getName());
			passportLineLock.Release();

			// wait for Customer data
			printf("%s waiting on transaction.\n",
					passportClerkLines[clerkIndex]->name);
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
			ASSERT(
					passportClerkLines[clerkIndex]->customer->appDone
							&& passportClerkLines[clerkIndex]->customer->picDone);
			passportClerkLines[clerkIndex]->customer->certified = true;
			passportClerkLines[clerkIndex]->transactionCV->Signal(
					passportClerkLines[clerkIndex]->transactionLock);
			printf("%s's application and picture are approved.\n",
					passportClerkLines[clerkIndex]->customer->name);
			printf("%s is done at %s. ",
					passportClerkLines[clerkIndex]->customer->name,
					passportClerkLines[clerkIndex]->name);

			printf("%s released transaction lock %s.\n",
					passportClerkLines[clerkIndex]->name,
					passportClerkLines[clerkIndex]->transactionLock->getName());
			passportClerkLines[clerkIndex]->transactionLock->Release();
		}
	}
}

void beCashier(int cashierIndex) {
	//-------------------------------------------------------------------
	// Cashier
	//-------------------------------------------------------------------
	while (true) {
		// once they are not on break, process the line
		while (cashierLines[cashierIndex]->state != Cashier::BREAK) {
			cashierLineLock.Acquire();
			printf("%s acquired %s.\n", cashierLines[cashierIndex]->name,
					cashierLineLock.getName());

			if (cashierLines[cashierIndex]->lineCount > 0) {
				printf(
						"%s is available and there is someone in line, signaling.\n",
						cashierLines[cashierIndex]->name);
				cashierLines[cashierIndex]->lineCV->Signal(&cashierLineLock);
				cashierLines[cashierIndex]->state = Cashier::BUSY;
			}
			else {
				printf("No one in line for %s.\n",
						cashierLines[cashierIndex]->name);
				cashierLines[cashierIndex]->state = Cashier::BREAK;
				printf("%s is now on break.\n",
						cashierLines[cashierIndex]->name);
				cashierLines[cashierIndex]->breakCV->Wait(
						cashierLines[cashierIndex]->breakLock);
				break; // lol get it break haha
			}

			cashierLines[cashierIndex]->transactionLock->Acquire();
			printf("%s acquired transaction lock %s.\n",
					cashierLines[cashierIndex]->name,
					cashierLines[cashierIndex]->transactionLock->getName());

			printf("%s released %s.\n", cashierLines[cashierIndex]->name,
					cashierLineLock.getName());
			cashierLineLock.Release();

			// wait for Customer data
			printf("%s waiting on transaction.\n",
					cashierLines[cashierIndex]->name);
			cashierLines[cashierIndex]->transactionCV->Wait(
					cashierLines[cashierIndex]->transactionLock);

			// Doing job, customer waiting, signal when done
			printf("%s accepting money from %s.\n",
					cashierLines[cashierIndex]->name,
					cashierLines[cashierIndex]->customer->name);
			// receive money from customer
			cashierLines[cashierIndex]->money += 100;
			cashierLines[cashierIndex]->customer->money -= 100;
			// set passport as received by customer
			cashierLines[cashierIndex]->customer->gotPassport = true;
			cashierLines[cashierIndex]->transactionCV->Signal(
					cashierLines[cashierIndex]->transactionLock);

			printf("%s's money has been accepted.\n",
					cashierLines[cashierIndex]->customer->name);

			printf("%s is done at %s. ",
					cashierLines[cashierIndex]->customer->name,
					cashierLines[cashierIndex]->name);

			printf("%s released transaction lock %s.\n",
					cashierLines[cashierIndex]->name,
					cashierLines[cashierIndex]->transactionLock->getName());
			cashierLines[cashierIndex]->transactionLock->Release();
		}
	}
}

// The customer functionality
void beCustomer(int customerIndex) {
//-------------------------------------------------------------------
// Step 1: choosing shortest of App and Pic line
//-------------------------------------------------------------------
	int myLine;
	int lineSize;
	int chosePic = -1; //1 if they chose a picture line (true), 0 if they chose an applicaiton line (false)
	Lock * targetLock;

	printf("%s starting customer process.\n", currentThread->getName());
	while (!customers[customerIndex]->picDone
			|| !customers[customerIndex]->appDone) {
		myLine = -1;
		lineSize = 1000;

		if (!customers[customerIndex]->picDone
				&& !customers[customerIndex]->appDone) {

			picLineLock.Acquire();
			printf("%s acquired %s.\n", currentThread->getName(),
					picLineLock.getName());

			appLineLock.Acquire();
			printf("%s acquired %s.\n", currentThread->getName(),
					appLineLock.getName());

			//Customer has a 50-50 shot of choosing the pic or app line
			if (rand() % 2 == 0) { //Customer has picked the PICTURE line

				//we can release the appLineLock becuase we are only concerned with Picture Clerk Lines
				printf(
						"%s releasing %s because it has chosen to get its picture taken first.\n",
						currentThread->getName(), appLineLock.getName());
				appLineLock.Release();

				chosePic = 1;

				for (unsigned int i = 0; i < ARRAY_SIZE(picClerkLines); ++i) {
					if (picClerkLines[i]->regularLineCount
							+ picClerkLines[i]->bribeLineCount < lineSize) {
						myLine = i;
						lineSize = picClerkLines[i]->regularLineCount
								+ picClerkLines[i]->bribeLineCount;
					}
				}

			}
			else { //Customer has picked the APPLICATION line

				//we can release the pictureLineLock becuase we are only concerned with App Clerk Lines
				printf(
						"%s releasing %s because it has chosen to submit its application first.\n",
						currentThread->getName(), picLineLock.getName());
				picLineLock.Release();

				chosePic = 0;

				for (unsigned int i = 0; i < ARRAY_SIZE(appClerkLines); ++i) {
					if (appClerkLines[i]->regularLineCount
							+ appClerkLines[i]->bribeLineCount < lineSize) {
						myLine = i;
						lineSize = appClerkLines[i]->regularLineCount
								+ appClerkLines[i]->bribeLineCount;
					}
				}

			}
		}
		else if (!customers[customerIndex]->picDone) { //Customer has submitted app but not taken photo

			picLineLock.Acquire();
			printf(
					"%s acquired %s, as they have already submitted their app.\n",
					currentThread->getName(), picLineLock.getName());

			chosePic = 1;

			for (unsigned int i = 0; i < ARRAY_SIZE(picClerkLines); ++i) {
				if (picClerkLines[i]->regularLineCount
						+ picClerkLines[i]->bribeLineCount < lineSize) {
					myLine = i;
					lineSize = picClerkLines[i]->regularLineCount
							+ picClerkLines[i]->bribeLineCount;
				}
			}
		}
		else if (!customers[customerIndex]->appDone) { //Customer has taken photo but not submitted app

			appLineLock.Acquire();
			printf(
					"%s acquired %s, as they have already had their photo taken.\n",
					currentThread->getName(), appLineLock.getName());

			chosePic = 0;

			for (unsigned int i = 0; i < ARRAY_SIZE(appClerkLines); ++i) {
				if (appClerkLines[i]->regularLineCount
						+ appClerkLines[i]->bribeLineCount < lineSize) {
					myLine = i;
					lineSize = appClerkLines[i]->regularLineCount
							+ appClerkLines[i]->bribeLineCount;
				}
			}
		}

		if (chosePic == 1) { //Customer is trying to take Picture

			printf("From the possible clerks, %s chose %s.\n",
					customers[customerIndex]->name,
					picClerkLines[myLine]->name);

			//Must wait for Clerk to be available
			if (picClerkLines[myLine]->state != Clerk::AVAILABLE) {
				if (customers[customerIndex]->money >= 600) {
					//IF enough money, 20% chance of bribing
					int bribeChance = rand() % 5;

					//decided to bribe
					if (bribeChance == 0) {
						picClerkLines[myLine]->bribeLineCount++;
						printf("%s has gotten in bribe line for %s.\n",
								customers[customerIndex]->name,
								picClerkLines[myLine]->name);
						customers[customerIndex]->money -= 500;
						picClerkLines[myLine]->bribeLineCV->Wait(&picLineLock);
						picClerkLines[myLine]->bribeLineCount--;
					}
					//did not decide to bribe
					else {
						picClerkLines[myLine]->regularLineCount++;
						printf("%s has gotten in regular line for %s.\n",
								customers[customerIndex]->name,
								picClerkLines[myLine]->name);
						picClerkLines[myLine]->regularLineCV->Wait(
								&picLineLock);
						picClerkLines[myLine]->regularLineCount--;
					}

				}
				else {
					picClerkLines[myLine]->regularLineCount++;
					printf("%s has gotten in regular line for %s.\n",
							customers[customerIndex]->name,
							picClerkLines[myLine]->name);
					picClerkLines[myLine]->regularLineCV->Wait(&picLineLock);
					picClerkLines[myLine]->regularLineCount--;
				}
			}

			// Clerk is now available, current customer can approach the clerk.
			//		ASSERT(clerkLines[myLine]->state != Clerk::BUSY);
			picClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

			//		printf("%s acquired lock %s.\n", currentThread->getName(), clerkLines[myLine]->lock->getName());
			//		clerkLines[myLine]->lock->Acquire();

			/*printf("%s releasing %s\n", customers[customerIndex]->name,
			 picLineLock.getName());*/

			// interaction begins
			picClerkTransaction(customerIndex, myLine);
		}
		else { //Customer is trying to submit Application

			/*printf("%s chose %s.\n", customers[customerIndex]->name,
			 appClerkLines[myLine]->name);*/

			//Must wait for Clerk to be available
			if (appClerkLines[myLine]->state != Clerk::AVAILABLE) {

				if (customers[customerIndex]->money >= 600) {
					//IF enough money, 20% chance of bribing
					int bribeChance = rand() % 5;

					//decided to bribe
					if (bribeChance == 0) {
						appClerkLines[myLine]->bribeLineCount++;
						printf("%s has gotten in bribe line for %s.\n",
								customers[customerIndex]->name,
								appClerkLines[myLine]->name);
						customers[customerIndex]->money -= 500;
						appClerkLines[myLine]->bribeLineCV->Wait(&appLineLock);
						appClerkLines[myLine]->bribeLineCount--;
					}
					else { //did not decide to bribe
						appClerkLines[myLine]->regularLineCount++;
						printf("%s has gotten in regular line for %s.\n",
								customers[customerIndex]->name,
								appClerkLines[myLine]->name);
						appClerkLines[myLine]->regularLineCV->Wait(
								&appLineLock);
						appClerkLines[myLine]->regularLineCount--;
					}
				}
				else { //not enough funds to bribe
					appClerkLines[myLine]->regularLineCount++;
					printf("%s has gotten in regular line for %s.\n",
							customers[customerIndex]->name,
							appClerkLines[myLine]->name);
					appClerkLines[myLine]->regularLineCV->Wait(&appLineLock);
					appClerkLines[myLine]->regularLineCount--;
				}
			}

			// Clerk is now available, current customer can approach the clerk.
			//		ASSERT(clerkLines[myLine]->state != Clerk::BUSY);
			appClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

			//		printf("%s acquired lock %s.\n", currentThread->getName(), clerkLines[myLine]->lock->getName());
			//		clerkLines[myLine]->lock->Acquire();

			printf("%s releasing %s\n", customers[customerIndex]->name,
					appLineLock.getName());
			appLineLock.Release();
			// interaction begins
			appClerkTransaction(customerIndex, myLine);
		}
	}

	printf("%s finished picture and application tasks.\n",
			currentThread->getName());
//-------------------------------------------------------------------
// Step 2: Going to passport clerk
//-------------------------------------------------------------------
	while (customers[customerIndex]->appDone
			&& customers[customerIndex]->picDone
			&& !customers[customerIndex]->certified) {
		// choose shortest passport clerk line
		passportLineLock.Acquire();
		myLine = -1;
		lineSize = 1000;

		//TODO: need to decide whether to bribe. Assumes regular line right now.
		for (unsigned int i = 0; i < ARRAY_SIZE(passportClerkLines); ++i) {
			if (passportClerkLines[i]->regularLineCount < lineSize) {
				myLine = i;
				lineSize = passportClerkLines[i]->regularLineCount;
			}
		}

		printf("%s chose %s.\n", customers[customerIndex]->name,
				passportClerkLines[myLine]->name);
		// Customer must wait for clerk to become available.

		if (passportClerkLines[myLine]->state != Clerk::AVAILABLE) {
			if (customers[customerIndex]->money >= 600) {
				//IF enough money, 20% chance of bribing
				int bribeChance = rand() % 5;
				if (bribeChance == 0) { //decided to bribe
					passportClerkLines[myLine]->bribeLineCount++;
					printf("%s has gotten in bribe line for %s.\n",
							customers[customerIndex]->name,
							passportClerkLines[myLine]->name);
					customers[customerIndex]->money -= 500;
					passportClerkLines[myLine]->bribeLineCV->Wait(
							&passportLineLock);
					passportClerkLines[myLine]->bribeLineCount--;
				}
				else { //decided not to bribe
					passportClerkLines[myLine]->regularLineCount++;
					printf("%s has gotten in regular line for %s.\n",
							customers[customerIndex]->name,
							passportClerkLines[myLine]->name);
					passportClerkLines[myLine]->regularLineCV->Wait(
							&passportLineLock);
					passportClerkLines[myLine]->regularLineCount--;
				}
			}
			else { //insufficient funds to bribe
				passportClerkLines[myLine]->regularLineCount++;
				printf("%s has gotten in regular line for %s.\n",
						customers[customerIndex]->name,
						passportClerkLines[myLine]->name);
				passportClerkLines[myLine]->regularLineCV->Wait(
						&passportLineLock);
				passportClerkLines[myLine]->regularLineCount--;
			}
		}

		// Clerk is now available, current customer can approach the clerk.
		passportClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

		printf("%s releasing %s\n", customers[customerIndex]->name,
				passportLineLock.getName());
		passportLineLock.Release();
		// interaction begins
		passportClerkTransaction(customerIndex, myLine);
	}
//-------------------------------------------------------------------
// Step 3: Going to cashier
//-------------------------------------------------------------------
	while (customers[customerIndex]->certified
			&& !customers[customerIndex]->gotPassport) {
		// choose shortest cashier line
		cashierLineLock.Acquire();
		myLine = -1;
		lineSize = 1000;

		//TODO: need to decide whether to bribe. Assumes regular line right now.
		for (unsigned int i = 0; i < ARRAY_SIZE(cashierLines); ++i) {
			if (cashierLines[i]->lineCount < lineSize) {
				myLine = i;
				lineSize = cashierLines[i]->lineCount;
			}
		}

		printf("%s chose %s.\n", customers[customerIndex]->name,
				cashierLines[myLine]->name);
		// Customer must wait for cashier to become available.

		if (cashierLines[myLine]->state != Cashier::AVAILABLE) {
			cashierLines[myLine]->lineCount++;
			printf("%s has gotten in line for %s.\n",
					customers[customerIndex]->name, cashierLines[myLine]->name);
			cashierLines[myLine]->lineCV->Wait(&cashierLineLock);
			cashierLines[myLine]->lineCount--;
		}

		// Cashier is now available, current customer can approach the cashier.
		cashierLines[myLine]->state = Cashier::BUSY; // cashier is now busy

		cashierLineLock.Release();
		// interaction begins
		cashierTransaction(customerIndex, myLine);
	}

	printf("%s is leaving the Passport Office.\n",
			customers[customerIndex]->name);
}

void PassportOffice() {
	Thread * t;
	char * name;
	int i;

	srand(time(NULL));

	passportClerkLines[0] = new Clerk("Passport Clerk 0", Clerk::PP);

	cashierLines[0] = new Cashier("Cashier 0");

	picClerkLines[0] = new Clerk("Pic Clerk 0", Clerk::PIC);
	appClerkLines[0] = new Clerk("App Clerk 0", Clerk::APP);

	managers[0] = new Manager("Manager 0");

	for (i = 0; i < 1; ++i) {
		name = passportClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePassportClerk, i);
	}

	for (i = 0; i < 1; ++i) {
		name = cashierLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCashier, i);
	}

	for (i = 0; i < 1; ++i) {
		name = managers[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beManager, i);
	}

	for (i = 0; i < 1; ++i) {
		name = picClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePicClerk, i);
	}

	for (i = 0; i < 1; ++i) {
		name = appClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beAppClerk, i);
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

