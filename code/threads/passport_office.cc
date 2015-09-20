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

void broadcastMoney();

struct Customer {
	enum customerType {
		REGULAR, SENATOR
	};
	char * name;
	int SSN;
	bool picDone;
	bool appDone;
	bool certified;
	bool gotPassport;
	int money;
	customerType type;

	Customer(char * n, int ssn, customerType t) {
		name = n;
		picDone = false;
		appDone = false;
		certified = false;
		gotPassport = false;
		type = t;
		SSN = ssn;
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
	int index;
	bool approved;
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
		index = -1;
		approved = false;
		regularLineCount = 0;
		bribeLineCount = 0;
		state = AVAILABLE;
		type = APP;

		regularLineCV = new Condition(" regular line CV");

		bribeLineCV = new Condition(" bribe line CV");

		transactionCV = new Condition(" transaction CV");

		transactionLock = new Lock(" transaction Locks");

		customer = NULL;
		money = 0;

		breakCV = new Condition(" break CV");
		breakLock = new Lock(" break lock");
	}

	Clerk(char * n, int i, clerkType t) {
		name = new char[50];
		sprintf(name, n);
		index = i;
		sprintf(name, strcat(name, "%i"), index);

		regularLineCount = 0;
		bribeLineCount = 0;
		state = AVAILABLE;
		type = t;

		char * temp = new char[50];
		strcpy(temp, name);
		regularLineCV = new Condition(strcat(temp, " regular line CV"));

		temp = new char[50];
		strcpy(temp, name);
		bribeLineCV = new Condition(strcat(temp, " bribe line CV"));

		temp = new char[50];
		strcpy(temp, name);
		transactionCV = new Condition(strcat(temp, " transaction CV"));

		temp = new char[50];
		strcpy(temp, name);
		transactionLock = new Lock(strcat(temp, " transaction Lock"));

		customer = NULL;
		approved = false;
		money = 0;

		temp = new char[50];
		strcpy(temp, name);
		breakCV = new Condition(strcat(temp, " break CV"));
		temp = new char[50];
		strcpy(temp, name);
		breakLock = new Lock(strcat(temp, " break lock"));
	}
};

class Manager {
public:
	char * name;
	int index;
	int counter;

	Manager(char * n, int i) {
		name = new char[50];
		sprintf(name, n);
		index = i;
		sprintf(name, strcat(name, "%i"), index);
		counter = 0;
	}
};

struct Cashier {
public:
	enum cashierState {
		AVAILABLE, BUSY, BREAK
	};
	char * name;
	int index;
	bool approved;
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
		index = -1;
		lineCount = 0;
		state = AVAILABLE;
		approved = false;
		lineCV = new Condition(" line CV");

		transactionCV = new Condition(" transaction CV");

		transactionLock = new Lock(" transaction Lock");

		customer = NULL;

		breakCV = new Condition(" break CV");
		breakLock = new Lock("break lock");
	}

	Cashier(char * n, int i) {
		name = new char[50];
		sprintf(name, n);
		index = i;
		sprintf(name, strcat(name, "%i"), index);

		lineCount = 0;
		approved = false;
		state = AVAILABLE;

		char * temp = new char[50];
		strcpy(temp, name);
		lineCV = new Condition(strcat(temp, " line CV"));

		temp = new char[50];
		strcpy(temp, name);
		transactionCV = new Condition(strcat(temp, " transaction CV"));

		temp = new char[50];
		strcpy(temp, name);
		transactionLock = new Lock(strcat(temp, " transaction Lock"));
		customer = NULL;

		temp = new char[50];
		strcpy(temp, name);
		breakCV = new Condition(strcat(temp, " break CV"));
		temp = new char[50];
		strcpy(temp, name);
		breakLock = new Lock(strcat(temp, " break lock"));
	}
};

// Customer's first step, either app or pic clerk
Lock picLineLock("Pic Line Lock");
Lock appLineLock("App Line Lock");
Lock passportLineLock("Passport Line Lock");
Lock cashierLineLock("Cashier Line Lock");

Lock senatorLock("Senator lock");
Semaphore senatorSema("Senator Semaphore", 1);
Condition senatorCV("Senator CV");

unsigned int NUM_CUSTOMERS = 0;
unsigned int NUM_PIC_CLERKS = 0;
unsigned int NUM_APP_CLERKS = 0;
unsigned int NUM_PP_CLERKS = 0;
unsigned int NUM_CASHIERS = 0;
unsigned int NUM_MANAGERS = 0;

bool senatorInProcess = false;

Clerk * picClerkLines[100];
Clerk * appClerkLines[100];
Clerk * passportClerkLines[100];
Cashier * cashierLines[100];

Manager * managers[100];

Customer * customers[100];

// test case flags
bool case1 = false;
bool case2 = false;
bool case3 = false;
bool case4 = false;
bool case5 = false;
bool case6 = false;
bool case7 = false;

void broadcastMoney() {
	// TODO add the rest of the locks
	int officeTotal = 0;
	int appClerkTotal = 0;
	int picClerkTotal = 0;
	int passClerkTotal = 0;
	int cashierTotal = 0;

	// Read all of the clerks/cashiers and print their sums.
	for (unsigned int i = 0; i < NUM_PIC_CLERKS; ++i) {
		picClerkLines[i]->transactionLock->Acquire();
		picClerkTotal += picClerkLines[i]->money;
	}

	for (unsigned int i = 0; i < NUM_APP_CLERKS; ++i) {
		appClerkLines[i]->transactionLock->Acquire();
		appClerkTotal += appClerkLines[i]->money;
	}

	for (unsigned int i = 0; i < NUM_PP_CLERKS; ++i) {
		passportClerkLines[i]->transactionLock->Acquire();
		passClerkTotal += passportClerkLines[i]->money;
	}

	for (unsigned int i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i]->transactionLock->Acquire();
		cashierTotal += cashierLines[i]->money;
	}

	officeTotal = appClerkTotal + picClerkTotal + passClerkTotal + cashierTotal;
	printf(
			"\nManager has counted amounts of:\n $%i for PictureClerks\n $%i for ApplicationClerks\n $%i for PassportClerks\n $%i for Cashiers\n Grand total is $%i\n\n",
			picClerkTotal, appClerkTotal, passClerkTotal, cashierTotal,
			officeTotal);

	for (unsigned int i = 0; i < NUM_PIC_CLERKS; ++i) {
		picClerkLines[i]->transactionLock->Release();
	}

	for (unsigned int i = 0; i < NUM_APP_CLERKS; ++i) {
		appClerkLines[i]->transactionLock->Release();
	}

	for (unsigned int i = 0; i < NUM_PP_CLERKS; ++i) {
		passportClerkLines[i]->transactionLock->Release();
	}

	for (unsigned int i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i]->transactionLock->Release();
	}
}

void beManager(int index) {
	while (true) {
//		if (managers[index]->counter % 2 == 0) {
//			broadcastMoney();
//		}
		managers[index]->counter++;
		for (unsigned int i = 0; i < NUM_PIC_CLERKS; ++i) {
			picLineLock.Acquire();
			picClerkLines[i]->transactionLock->Acquire();
			picClerkLines[i]->breakLock->Acquire();
			if (picClerkLines[i]->state == Clerk::BREAK
					&& (picClerkLines[i]->bribeLineCount
							+ picClerkLines[i]->regularLineCount) > 3) {
				picClerkLines[i]->state = Clerk::AVAILABLE;
				picClerkLines[i]->breakCV->Signal(picClerkLines[i]->breakLock);
				printf("Manager has woken up a PictureClerk\n");
			}
			picClerkLines[i]->breakLock->Release();
			picClerkLines[i]->transactionLock->Release();
			picLineLock.Release();
		}
		for (unsigned int i = 0; i < NUM_APP_CLERKS; ++i) {
			appLineLock.Acquire();
			appClerkLines[i]->transactionLock->Acquire();
			appClerkLines[i]->breakLock->Acquire();
			if (appClerkLines[i]->state == Clerk::BREAK
					&& (appClerkLines[i]->bribeLineCount
							+ appClerkLines[i]->regularLineCount) > 3) {
				appClerkLines[i]->state = Clerk::AVAILABLE;
				appClerkLines[i]->breakCV->Signal(appClerkLines[i]->breakLock);
				printf("Manager has woken up an ApplicationClerk\n");
			}
			appClerkLines[i]->breakLock->Release();
			appClerkLines[i]->transactionLock->Release();
			appLineLock.Release();
		}
		for (unsigned int i = 0; i < NUM_PP_CLERKS; ++i) {
			passportLineLock.Acquire();
			passportClerkLines[i]->breakLock->Acquire();
			passportClerkLines[i]->transactionLock->Acquire();
			if (passportClerkLines[i]->state == Clerk::BREAK
					&& (passportClerkLines[i]->bribeLineCount
							+ passportClerkLines[i]->regularLineCount) > 3) {
				passportClerkLines[i]->state = Clerk::AVAILABLE;
				passportClerkLines[i]->breakCV->Signal(
						passportClerkLines[i]->breakLock);
				printf("Manager has woken up a PassportClerk\n");
			}
			passportClerkLines[i]->breakLock->Release();
			passportClerkLines[i]->transactionLock->Release();
			passportLineLock.Release();
		}
		for (unsigned int i = 0; i < NUM_CASHIERS; ++i) {
			cashierLineLock.Acquire();
			cashierLines[i]->transactionLock->Acquire();
			cashierLines[i]->breakLock->Acquire();
			if (cashierLines[i]->state == Cashier::BREAK
					&& cashierLines[i]->lineCount > 0) {
				cashierLines[i]->state = Cashier::AVAILABLE;
				cashierLines[i]->breakCV->Signal(cashierLines[i]->breakLock);
				printf("Manager has woken up a Cashier\n");
			}
			cashierLines[i]->breakLock->Release();
			cashierLines[i]->transactionLock->Release();
			cashierLineLock.Release();
		}
		for (int i = 0; i < 5000; ++i)
			currentThread->Yield();
	}
}

void picClerkTransaction(int customer, int clerk) {
	// Set the clerk's current customer
	picClerkLines[clerk]->transactionLock->Acquire();
	picClerkLines[clerk]->customer = customers[customer];

	// send SSN
	printf("%s has given SSN %i to %s\n", customers[customer]->name,
			customers[customer]->SSN, picClerkLines[clerk]->name);
	picClerkLines[clerk]->transactionCV->Signal(
			picClerkLines[clerk]->transactionLock);

	// take picture

	// Wait to be shown picture
	// Take picture until he likes it
	while (!customers[customer]->picDone) {
		picClerkLines[clerk]->transactionCV->Wait(
				picClerkLines[clerk]->transactionLock);

		if ((rand() % 10) % 2 == 0) {
			printf("%s does not like their picture from %s.\n",
					currentThread->getName(), picClerkLines[clerk]->name);
			picClerkLines[clerk]->transactionCV->Signal(
					picClerkLines[clerk]->transactionLock);
		}
		else {
			printf("%s does like their picture from %s.\n",
					currentThread->getName(), picClerkLines[clerk]->name);
			customers[customer]->picDone = true;
			picClerkLines[clerk]->transactionCV->Signal(
					picClerkLines[clerk]->transactionLock);
		}
	}
	picClerkLines[clerk]->transactionCV->Wait(
			picClerkLines[clerk]->transactionLock);

	printf("%s is leaving %s's counter.\n", currentThread->getName(),
			picClerkLines[clerk]->name);
	picClerkLines[clerk]->transactionCV->Signal(
			picClerkLines[clerk]->transactionLock);
	picClerkLines[clerk]->transactionLock->Release();
}

void appClerkTransaction(int customer, int clerk) {
	// Set the clerk's current customer
	appClerkLines[clerk]->transactionLock->Acquire();
	appClerkLines[clerk]->customer = customers[customer];
	printf("%s has given SSN %i to %s\n", customers[customer]->name,
			customers[customer]->SSN, appClerkLines[clerk]->name);
	appClerkLines[clerk]->transactionCV->Signal(
			appClerkLines[clerk]->transactionLock);

	/*printf("%s waiting for clerk %s to file the application.\n",
	 currentThread->getName(), appClerkLines[clerk]->name);*/
	appClerkLines[clerk]->transactionCV->Wait(
			appClerkLines[clerk]->transactionLock);

	appClerkLines[clerk]->transactionCV->Signal(
			appClerkLines[clerk]->transactionLock);

	/*printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
	 appClerkLines[clerk]->name);*/
	printf("%s is leaving %s's counter.\n", currentThread->getName(),
			appClerkLines[clerk]->name);
	appClerkLines[clerk]->transactionLock->Release();
}

void passportClerkTransaction(int customer, int clerk) {
	passportClerkLines[clerk]->transactionLock->Acquire();
	passportClerkLines[clerk]->customer = customers[customer];

	passportClerkLines[clerk]->transactionCV->Signal(
			passportClerkLines[clerk]->transactionLock);

	passportClerkLines[clerk]->transactionCV->Wait(
			passportClerkLines[clerk]->transactionLock);

	if (passportClerkLines[clerk]->approved) {
		printf("%s has given SSN %i to %s\n", customers[customer]->name,
				customers[customer]->SSN, passportClerkLines[clerk]->name);

		passportClerkLines[clerk]->transactionCV->Signal(
				passportClerkLines[clerk]->transactionLock);

		passportClerkLines[clerk]->transactionCV->Wait(
				passportClerkLines[clerk]->transactionLock);
	}

//	ASSERT(customers[customer]->appDone && customers[customer]->picDone);
	printf("%s is leaving %s's counter.\n", currentThread->getName(),
			passportClerkLines[clerk]->name);
	passportClerkLines[clerk]->transactionCV->Signal(
			passportClerkLines[clerk]->transactionLock);
	passportClerkLines[clerk]->transactionLock->Release();
}

void cashierTransaction(int customer, int cashier) {
	// Set the cashier's current customer
	cashierLines[cashier]->transactionLock->Acquire();
	cashierLines[cashier]->customer = customers[customer];

	cashierLines[cashier]->transactionCV->Signal(
			cashierLines[cashier]->transactionLock);

	cashierLines[cashier]->transactionCV->Wait(
			cashierLines[cashier]->transactionLock);

	if (cashierLines[cashier]->approved) {
		cashierLines[cashier]->transactionCV->Signal(
				cashierLines[cashier]->transactionLock);

		printf("%s has given SSN %i to %s\n", customers[customer]->name,
				customers[customer]->SSN, cashierLines[cashier]->name);
		cashierLines[cashier]->transactionCV->Wait(
				cashierLines[cashier]->transactionLock);

		printf("%s has given %s $100.\n", currentThread->getName(),
				cashierLines[cashier]->name);
		cashierLines[cashier]->transactionCV->Signal(
				cashierLines[cashier]->transactionLock);
		cashierLines[cashier]->transactionCV->Wait(
				cashierLines[cashier]->transactionLock);
	}

	printf("%s is leaving %s's counter.\n", currentThread->getName(),
			cashierLines[cashier]->name);
	cashierLines[cashier]->transactionCV->Signal(
			cashierLines[cashier]->transactionLock);
	cashierLines[cashier]->transactionLock->Release();
}

void bePicClerk(int clerkIndex) {
	//-------------------------------------------------------------------
	// Picture Clerk
	//-------------------------------------------------------------------
	while (true) {
		while (picClerkLines[clerkIndex]->state != Clerk::BREAK) {
			picLineLock.Acquire();
			/*printf("%s acquired %s.\n", picClerkLines[clerkIndex]->name,
			 picLineLock.getName());*/

			if (picClerkLines[clerkIndex]->bribeLineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->bribeLineCV->Signal(&picLineLock);
				picClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else if (picClerkLines[clerkIndex]->regularLineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->regularLineCV->Signal(&picLineLock);
				picClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else {
				picClerkLines[clerkIndex]->breakLock->Acquire();
				picLineLock.Release();
				printf("%s has no on in line.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->state = Clerk::BREAK;
				printf("%s is now on break.\n",
						picClerkLines[clerkIndex]->name);
				picClerkLines[clerkIndex]->breakCV->Wait(
						picClerkLines[clerkIndex]->breakLock);
				picClerkLines[clerkIndex]->breakLock->Release();
				break;
			}

			picClerkLines[clerkIndex]->transactionLock->Acquire();
			/*printf("%s acquired transaction lock %s.\n",
			 picClerkLines[clerkIndex]->name,
			 picClerkLines[clerkIndex]->transactionLock->getName());

			 printf("%s released %s.\n", picClerkLines[clerkIndex]->name,
			 picLineLock.getName());*/
			picLineLock.Release();

			// wait for Customer data
			picClerkLines[clerkIndex]->transactionCV->Wait(
					picClerkLines[clerkIndex]->transactionLock);

			printf("%s has received SSN %i from %s\n",
					picClerkLines[clerkIndex]->name,
					picClerkLines[clerkIndex]->customer->SSN,
					picClerkLines[clerkIndex]->customer->name);

			bool firstTime = true;
			// Doing job, customer waiting, signal when done
			while (!picClerkLines[clerkIndex]->customer->picDone) {
				if (!firstTime) {
					printf(
							"%s has been told that %s does not like their picture\n",
							picClerkLines[clerkIndex]->name,
							picClerkLines[clerkIndex]->customer->name);
				}
				printf("%s has taken a picture of %s.\n",
						picClerkLines[clerkIndex]->name,
						picClerkLines[clerkIndex]->customer->name);

				picClerkLines[clerkIndex]->transactionCV->Signal(
						picClerkLines[clerkIndex]->transactionLock);
				// Waiting for customer to accept photo
				picClerkLines[clerkIndex]->transactionCV->Wait(
						picClerkLines[clerkIndex]->transactionLock);

				firstTime = false;
			}

			printf("%s has been told that %s does like their picture\n",
					picClerkLines[clerkIndex]->name,
					picClerkLines[clerkIndex]->customer->name);

			/*printf("%s's picture is being filed.\n",
			 picClerkLines[clerkIndex]->customer->name);*/

			picClerkLines[clerkIndex]->transactionCV->Signal(
					picClerkLines[clerkIndex]->transactionLock);

			picClerkLines[clerkIndex]->transactionCV->Wait(
					picClerkLines[clerkIndex]->transactionLock);
			/*printf("%s's picture is now filed.\n",
			 picClerkLines[clerkIndex]->customer->name);
			 printf("%s is done at %s. ",
			 picClerkLines[clerkIndex]->customer->name,
			 picClerkLines[clerkIndex]->name);

			 printf("%s released transaction lock %s.\n",
			 picClerkLines[clerkIndex]->name,
			 picClerkLines[clerkIndex]->transactionLock->getName());*/
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
			/*printf("%s acquired %s.\n", appClerkLines[clerkIndex]->name,
			 appLineLock.getName());*/

			if (appClerkLines[clerkIndex]->bribeLineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->bribeLineCV->Signal(&appLineLock);
				appClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else if (appClerkLines[clerkIndex]->regularLineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->regularLineCV->Signal(&appLineLock);
				appClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else {
				appClerkLines[clerkIndex]->breakLock->Acquire();
				appLineLock.Release();
				printf("%s has no on in line.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->state = Clerk::BREAK;
				printf("%s is now on break.\n",
						appClerkLines[clerkIndex]->name);
				appClerkLines[clerkIndex]->breakCV->Wait(
						appClerkLines[clerkIndex]->breakLock);
				appClerkLines[clerkIndex]->breakLock->Release();
				break;
			}

			appClerkLines[clerkIndex]->transactionLock->Acquire();
			/*printf("%s acquired transaction lock %s.\n",
			 appClerkLines[clerkIndex]->name,
			 appClerkLines[clerkIndex]->transactionLock->getName());

			 printf("%s released %s.\n", appClerkLines[clerkIndex]->name,
			 appLineLock.getName());*/
			appLineLock.Release();

			// wait for Customer data
			/*printf("%s waiting on transaction.\n",
			 appClerkLines[clerkIndex]->name);*/
			appClerkLines[clerkIndex]->transactionCV->Wait(
					appClerkLines[clerkIndex]->transactionLock);

			printf("%s has received SSN %i from %s\n",
					appClerkLines[clerkIndex]->name,
					appClerkLines[clerkIndex]->customer->SSN,
					appClerkLines[clerkIndex]->customer->name);

			// Doing job, customer waiting, signal when done
			/*printf("%s filing application for %s.\n",
			 appClerkLines[clerkIndex]->name,
			 appClerkLines[clerkIndex]->customer->name);*/
			// Yield for a bit
			for (int i = 0; i < rand() % 80 + 20; ++i) {
				currentThread->Yield();
			}

			printf("%s has recorded a completed application for %s\n",
					appClerkLines[clerkIndex]->name,
					appClerkLines[clerkIndex]->customer->name);

			// set application as complete
			appClerkLines[clerkIndex]->customer->appDone = true;
			appClerkLines[clerkIndex]->transactionCV->Signal(
					appClerkLines[clerkIndex]->transactionLock);
			appClerkLines[clerkIndex]->transactionCV->Wait(
					appClerkLines[clerkIndex]->transactionLock);
			/*printf("%s's application is now filed.\n",
			 appClerkLines[clerkIndex]->customer->name);
			 printf("%s is done at %s. ",
			 appClerkLines[clerkIndex]->customer->name,
			 appClerkLines[clerkIndex]->name);

			 printf("%s released transaction lock %s.\n",
			 appClerkLines[clerkIndex]->name,
			 appClerkLines[clerkIndex]->transactionLock->getName());*/
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
			if (passportClerkLines[clerkIndex]->bribeLineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->bribeLineCV->Signal(
						&passportLineLock);
				passportClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else if (passportClerkLines[clerkIndex]->regularLineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->regularLineCV->Signal(
						&passportLineLock);
				passportClerkLines[clerkIndex]->state = Clerk::BUSY;
			}
			else {
				passportClerkLines[clerkIndex]->breakLock->Acquire();
				printf("%s has no on in line.\n",
						passportClerkLines[clerkIndex]->name);
				passportLineLock.Release();
				passportClerkLines[clerkIndex]->state = Clerk::BREAK;
				printf("%s is now on break.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->breakCV->Wait(
						passportClerkLines[clerkIndex]->breakLock);
				passportClerkLines[clerkIndex]->breakLock->Release();
				break;
			}

			passportClerkLines[clerkIndex]->transactionLock->Acquire();
			passportLineLock.Release();

			// wait for Customer data
			passportClerkLines[clerkIndex]->transactionCV->Wait(
					passportClerkLines[clerkIndex]->transactionLock);

			if (passportClerkLines[clerkIndex]->customer->picDone
					&& passportClerkLines[clerkIndex]->customer->appDone) {

				passportClerkLines[clerkIndex]->approved = true;
				printf("%s is approved to be certified.\n",
						passportClerkLines[clerkIndex]->name);

				passportClerkLines[clerkIndex]->transactionCV->Signal(
						passportClerkLines[clerkIndex]->transactionLock);
				passportClerkLines[clerkIndex]->transactionCV->Wait(
						passportClerkLines[clerkIndex]->transactionLock);
				printf("%s has received SSN %i from %s\n",
						passportClerkLines[clerkIndex]->name,
						passportClerkLines[clerkIndex]->customer->SSN,
						passportClerkLines[clerkIndex]->customer->name);

				// Doing job, customer waiting, signal when done

				// Yield for a bit
				for (int i = 0; i < rand() % 900 + 100; ++i) {
					currentThread->Yield();
				}
				// set application as complete
				passportClerkLines[clerkIndex]->customer->certified = true;
				passportClerkLines[clerkIndex]->transactionCV->Signal(
						passportClerkLines[clerkIndex]->transactionLock);

				printf(
						"%s has determined that %s has both their application and picture completed\n",
						passportClerkLines[clerkIndex]->name,
						passportClerkLines[clerkIndex]->customer->name);

				printf("%s has recorded %s passport documentation\n",
						passportClerkLines[clerkIndex]->name,
						passportClerkLines[clerkIndex]->customer->name);
			}
			else {
				passportClerkLines[clerkIndex]->approved = false;
				printf("%s is not approved to be certified -- Rejected.\n",
						passportClerkLines[clerkIndex]->name);
				passportClerkLines[clerkIndex]->transactionCV->Signal(
						passportClerkLines[clerkIndex]->transactionLock);

			}

			passportClerkLines[clerkIndex]->transactionCV->Wait(
					passportClerkLines[clerkIndex]->transactionLock);

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

			if (cashierLines[cashierIndex]->lineCount > 0) {
				printf(
						"%s has signalled a Customer to come to their counter.\n",
						cashierLines[cashierIndex]->name);
				cashierLines[cashierIndex]->lineCV->Signal(&cashierLineLock);
				cashierLines[cashierIndex]->state = Cashier::BUSY;
			}
			else {
				cashierLines[cashierIndex]->breakLock->Acquire();
				cashierLineLock.Release();
				printf("%s has no on in line.\n",
						cashierLines[cashierIndex]->name);
				cashierLines[cashierIndex]->state = Cashier::BREAK;
				printf("%s is now on break.\n",
						cashierLines[cashierIndex]->name);
				cashierLines[cashierIndex]->breakCV->Wait(
						cashierLines[cashierIndex]->breakLock);
				cashierLines[cashierIndex]->breakLock->Release();
				break; // lol get it break haha
			}

			cashierLines[cashierIndex]->transactionLock->Acquire();
			cashierLineLock.Release();

			// wait for Customer data
			cashierLines[cashierIndex]->transactionCV->Wait(
					cashierLines[cashierIndex]->transactionLock);

			// If the customer has finished everything
			if (cashierLines[cashierIndex]->customer->appDone
					&& cashierLines[cashierIndex]->customer->picDone
					&& cashierLines[cashierIndex]->customer->certified) {

				cashierLines[cashierIndex]->approved = true;
				printf("%s is certified to get their passport -- Approved.\n",
						cashierLines[cashierIndex]->customer->name);

				cashierLines[cashierIndex]->transactionCV->Signal(
						cashierLines[cashierIndex]->transactionLock);
				// wait for ssn
				cashierLines[cashierIndex]->transactionCV->Wait(
						cashierLines[cashierIndex]->transactionLock);

				printf("%s has received SSN %i from %s\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->SSN,
						cashierLines[cashierIndex]->customer->name);

				// got ssn, wait for money
				cashierLines[cashierIndex]->transactionCV->Signal(
						cashierLines[cashierIndex]->transactionLock);
				cashierLines[cashierIndex]->transactionCV->Wait(
						cashierLines[cashierIndex]->transactionLock);

				// Doing job, customer waiting, signal when done
				printf("%s has received the $100 from %s after certification\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);

				// receive money from customer
				cashierLines[cashierIndex]->money += 100;
				cashierLines[cashierIndex]->customer->money -= 100;
				// set passport as received by customer
				cashierLines[cashierIndex]->customer->gotPassport = true;
				cashierLines[cashierIndex]->transactionCV->Signal(
						cashierLines[cashierIndex]->transactionLock);

				printf("%s has provided %s their completed passport\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);

				printf(
						"%s has recorded that %s has been given their completed passport\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);
			}
			else {
				cashierLines[cashierIndex]->approved = false;
				printf(
						"%s is not yet certified to get their passport -- Rejected.\n",
						cashierLines[cashierIndex]->customer->name);
				cashierLines[cashierIndex]->transactionCV->Signal(
						cashierLines[cashierIndex]->transactionLock);
			}
			cashierLines[cashierIndex]->transactionCV->Wait(
					cashierLines[cashierIndex]->transactionLock);
			cashierLines[cashierIndex]->transactionLock->Release();
		}
	}
}

void picAppCustomerProcess(int customerIndex) {
	//--------------------------------------------------
	// if they havent done both, choose randomly
	//--------------------------------------------------
	int myLine = -1;
	int chosePic = -1;
	int lineSize = 1000;
	if (!customers[customerIndex]->picDone
			&& !customers[customerIndex]->appDone) {

		picLineLock.Acquire();
		appLineLock.Acquire();

		//Customer has a 50-50 shot of choosing the pic or app line
		if (rand() % 2 == 0) { //Customer has picked the PICTURE line

			//we can release the appLineLock becuase we are only concerned with Picture Clerk Lines
			printf(
					"%s releasing %s because it has chosen to get its picture taken first.\n",
					currentThread->getName(), appLineLock.getName());
			appLineLock.Release();

			chosePic = 1;

			for (unsigned int i = 0; i < NUM_PIC_CLERKS; ++i) {
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

			for (unsigned int i = 0; i < NUM_APP_CLERKS; ++i) {
				if (appClerkLines[i]->regularLineCount
						+ appClerkLines[i]->bribeLineCount < lineSize) {
					myLine = i;
					lineSize = appClerkLines[i]->regularLineCount
							+ appClerkLines[i]->bribeLineCount;
				}
			}
		}
	}
	//--------------------------------------------------
	// Other wise do the one that they need to do
	//--------------------------------------------------
	else if (!customers[customerIndex]->picDone) { //Customer has submitted app but not taken photo
		picLineLock.Acquire();

		chosePic = 1;

		for (unsigned int i = 0; i < NUM_PIC_CLERKS; ++i) {
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

		chosePic = 0;

		for (unsigned int i = 0; i < NUM_APP_CLERKS; ++i) {
			if (appClerkLines[i]->regularLineCount
					+ appClerkLines[i]->bribeLineCount < lineSize) {
				myLine = i;
				lineSize = appClerkLines[i]->regularLineCount
						+ appClerkLines[i]->bribeLineCount;
			}
		}
	}

	if (chosePic == 1) { //Customer is trying to take Picture
		/*printf("From the possible clerks, %s chose %s.\n",
		 customers[customerIndex]->name,
		 picClerkLines[myLine]->name);*/

		//Must wait for Clerk to be available
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
				picClerkLines[myLine]->money += 500;
				picClerkLines[myLine]->bribeLineCV->Wait(&picLineLock);
				picClerkLines[myLine]->bribeLineCount--;
			}
			//did not decide to bribe
			else {
				picClerkLines[myLine]->regularLineCount++;
				printf("%s has gotten in regular line for %s.\n",
						customers[customerIndex]->name,
						picClerkLines[myLine]->name);
				picClerkLines[myLine]->regularLineCV->Wait(&picLineLock);
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

		// Clerk is now available, current customer can approach the clerk.
		picClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

		// interaction begins
		picLineLock.Release();
		picClerkTransaction(customerIndex, myLine);
	}
	else { //Customer is trying to submit Application

		//Must wait for Clerk to be available

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
				picClerkLines[myLine]->money += 500;
				appClerkLines[myLine]->bribeLineCV->Wait(&appLineLock);
				appClerkLines[myLine]->bribeLineCount--;
			}
			else { //did not decide to bribe
				appClerkLines[myLine]->regularLineCount++;
				printf("%s has gotten in regular line for %s.\n",
						customers[customerIndex]->name,
						appClerkLines[myLine]->name);
				appClerkLines[myLine]->regularLineCV->Wait(&appLineLock);
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

		// Clerk is now available, current customer can approach the clerk.
		//		ASSERT(clerkLines[myLine]->state != Clerk::BUSY);
		appClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

		/*printf("%s releasing %s\n", customers[customerIndex]->name,
		 appLineLock.getName());*/
		appLineLock.Release();
		// interaction begins
		appClerkTransaction(customerIndex, myLine);
	}

	/*printf("%s finished picture and application tasks.\n",
	 currentThread->getName());*/
}

void passportCustomerProcess(int customerIndex) {
	// choose shortest passport clerk line
	passportLineLock.Acquire();
	int myLine = -1;
	int lineSize = 1000;

	//TODO: need to decide whether to bribe. Assumes regular line right now.
	for (unsigned int i = 0; i < NUM_PP_CLERKS; ++i) {
		if (passportClerkLines[i]->regularLineCount < lineSize) {
			myLine = i;
			lineSize = passportClerkLines[i]->regularLineCount;
		}
	}

	// Customer must wait for clerk to become available.
	if (customers[customerIndex]->money >= 600) {
		//IF enough money, 20% chance of bribing
		int bribeChance = rand() % 5;
		if (bribeChance == 0) { //decided to bribe
			passportClerkLines[myLine]->bribeLineCount++;
			printf("%s has gotten in bribe line for %s.\n",
					customers[customerIndex]->name,
					passportClerkLines[myLine]->name);
			customers[customerIndex]->money -= 500;
			picClerkLines[myLine]->money += 500;
			passportClerkLines[myLine]->bribeLineCV->Wait(&passportLineLock);
			passportClerkLines[myLine]->bribeLineCount--;
		}
		else { //decided not to bribe
			passportClerkLines[myLine]->regularLineCount++;
			printf("%s has gotten in regular line for %s.\n",
					customers[customerIndex]->name,
					passportClerkLines[myLine]->name);
			passportClerkLines[myLine]->regularLineCV->Wait(&passportLineLock);
			passportClerkLines[myLine]->regularLineCount--;
		}
	}
	else { //insufficient funds to bribe
		passportClerkLines[myLine]->regularLineCount++;
		printf("%s has gotten in regular line for %s.\n",
				customers[customerIndex]->name,
				passportClerkLines[myLine]->name);
		passportClerkLines[myLine]->regularLineCV->Wait(&passportLineLock);
		passportClerkLines[myLine]->regularLineCount--;
	}
	// Clerk is now available, current customer can approach the clerk.
	passportClerkLines[myLine]->state = Clerk::BUSY; // clerk is now busy

	passportLineLock.Release();
	// interaction begins
	passportClerkTransaction(customerIndex, myLine);
}

void cashierCustomerProcess(int customerIndex) {
	// choose shortest cashier line
	cashierLineLock.Acquire();
	int myLine = -1;
	int lineSize = 1000;

	//TODO: need to decide whether to bribe. Assumes regular line right now.
	for (unsigned int i = 0; i < NUM_CASHIERS; ++i) {
		if (cashierLines[i]->lineCount < lineSize) {
			myLine = i;
			lineSize = cashierLines[i]->lineCount;

		}
	}

	// Customer must wait for cashier to become available.
	cashierLines[myLine]->lineCount++;
	printf("%s has gotten in regular line for %s.\n",
			customers[customerIndex]->name, cashierLines[myLine]->name);
	cashierLines[myLine]->lineCV->Wait(&cashierLineLock);
	cashierLines[myLine]->lineCount--;

	// Cashier is now available, current customer can approach the cashier.
	cashierLines[myLine]->state = Cashier::BUSY; // cashier is now busy

	cashierLineLock.Release();
	// interaction begins
	cashierTransaction(customerIndex, myLine);
}

// The customer functionality
void beCustomer(int customerIndex) {
//-------------------------------------------------------------------
// Step 1: choosing shortest of App and Pic line
//-------------------------------------------------------------------
	Lock * targetLock;

	if (customers[customerIndex]->type == Customer::SENATOR) {
		senatorSema.P();
		senatorLock.Acquire();
		senatorInProcess = true;
	}

	while ((!customers[customerIndex]->picDone
			|| !customers[customerIndex]->appDone
			|| !customers[customerIndex]->certified
			|| !customers[customerIndex]->gotPassport)) {

		// if a regular customer and a senator is going
		if (customers[customerIndex]->type != Customer::SENATOR
				&& senatorInProcess) {
			senatorLock.Acquire();
			senatorCV.Wait(&senatorLock);
			senatorLock.Release();
		}
		if ((customers[customerIndex]->type == Customer::SENATOR
				&& senatorInProcess && senatorLock.isHeldByCurrentThread())
				|| !senatorInProcess) {

			//--------------------------------------------------
			// if the customer hasnt done one of these things
			//--------------------------------------------------
			if (!customers[customerIndex]->picDone
					|| !customers[customerIndex]->appDone) {
				picAppCustomerProcess(customerIndex);
			}
			//-------------------------------------------------------------------
			// Step 2: Going to passport clerk
			//-------------------------------------------------------------------
			else if (customers[customerIndex]->appDone
					&& customers[customerIndex]->picDone
					&& !customers[customerIndex]->certified) {
				passportCustomerProcess(customerIndex);
			}
			//-------------------------------------------------------------------
			// Step 3: Going to cashier
			//-------------------------------------------------------------------
			else if (customers[customerIndex]->certified
					&& !customers[customerIndex]->gotPassport) {
				cashierCustomerProcess(customerIndex);
			}

			// if there's a senator, you gotta wait
			if (customers[customerIndex]->type != Customer::SENATOR
					&& senatorInProcess) {
				senatorCV.Wait(&senatorLock);
			}
		}
	}
	printf("%s is leaving the Passport Office.\n",
			customers[customerIndex]->name);

	// the senator finishes up, so broadcast everyone the coast is clear
	if (customers[customerIndex]->type == Customer::SENATOR
			&& senatorInProcess) {
		senatorInProcess = false;
		senatorCV.Broadcast(&senatorLock);
		senatorLock.Release();
		senatorSema.V();
	}
}

//----------------------------------------------------------------
// "Customers always take the shortest line, but
// 	no 2 customers ever choose the same shortest line at the same time"
//
//	Each clerk and cashier is given an artificial line count.
//----------------------------------------------------------------
void testCase1() {
	Thread * t;
	char * name;
	unsigned int i;

	srand(time(NULL));

	NUM_CUSTOMERS = 2;
	NUM_PIC_CLERKS = 2;
	NUM_APP_CLERKS = 2;
	NUM_PP_CLERKS = 2;
	NUM_CASHIERS = 2;
	NUM_MANAGERS = 2;

	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		customers[i] = new Customer(name, i, Customer::REGULAR);
		printf("%s has just entered the passport office.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCustomer, i);
	}

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		passportClerkLines[i] = new Clerk("Passport Clerk ", i, Clerk::PP);

		passportClerkLines[i]->bribeLineCount = rand() % 10;
		passportClerkLines[i]->regularLineCount = rand() % 10;
		printf("%s : %i ppl in line.\n", passportClerkLines[i]->name,
				passportClerkLines[i]->regularLineCount
						+ passportClerkLines[i]->bribeLineCount);

		name = passportClerkLines[i]->name;
		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePassportClerk, i);
	}

	for (i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i] = new Cashier("Cashier ", i);
		cashierLines[i]->lineCount = i;
//		printf("%s : %i ppl in line.\n", cashierLines[i]->name,
//				cashierLines[i]->lineCount);

		name = cashierLines[i]->name;
		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCashier, i);
	}

	for (i = 0; i < NUM_MANAGERS; ++i) {
		managers[i] = new Manager("Manager ", i);

		name = managers[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beManager, i);
	}

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		picClerkLines[i] = new Clerk("Pic Clerk ", i, Clerk::PIC);

//		picClerkLines[i]->bribeLineCount = rand()%10;
//		picClerkLines[i]->regularLineCount = rand()%10;
		printf("%s : %i ppl in line.\n", picClerkLines[i]->name,
				picClerkLines[i]->bribeLineCount
						+ picClerkLines[i]->regularLineCount);

		name = picClerkLines[i]->name;
		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePicClerk, i);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		appClerkLines[i] = new Clerk("Application Clerk ", i, Clerk::APP);

//		appClerkLines[i]->bribeLineCount = rand()%10;
//		appClerkLines[i]->regularLineCount = rand()%10;
		printf("%s : %i ppl in line.\n", appClerkLines[i]->name,
				appClerkLines[i]->regularLineCount
						+ appClerkLines[i]->bribeLineCount);

		name = appClerkLines[i]->name;
		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beAppClerk, i);
	}

}

//----------------------------------------------------------------
// "Customers do not leave until they are given their passport by
// the Cashier. The Cashier does not start on another customer until they know that the last Customer has left their area"
//
//	This test is demonstrated with cashier. Upon creation, each
// customer has $100 and is randomly set to have or have not completed the first three
// steps (picture, application, and certification).

// There is only one cashier on duty. Each customer will get in line for this cashier,
// The cashier signals the customer to the counter, they complete the transaction,
// and then the customer leaves IF they have completed the previous steps.

// If the customer has not completed the three steps, they are rejected.

// A customer only approaches the counter once the previous customer has left
// and the cashier calls to them.
//----------------------------------------------------------------
void testCase3() {
	Thread * t;
	char * name;
	unsigned int i;

	srand(time(NULL));

	NUM_CUSTOMERS = 10;
	NUM_CASHIERS = 1;
	NUM_MANAGERS = 2;

	for (i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i] = new Cashier("Cashier ", i);
		cashierLines[i]->lineCount = i;
		name = cashierLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCashier, i);
	}

	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		customers[i] = new Customer(name, i, Customer::REGULAR);
		customers[i]->picDone = true;
		customers[i]->appDone = true;
		customers[i]->certified = (i % 2 == 0 ? true : false);
		customers[i]->money = 100;
		t->Fork((VoidFunctionPtr) cashierCustomerProcess, i);
	}
	for (i = 0; i < NUM_MANAGERS; ++i) {
		managers[i] = new Manager("Manager ", i);

		name = managers[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beManager, i);
	}
}

//----------------------------------------------------------------
// "Clerks go on break when they have no one waiting in their line"
//
// This test creates clerks of all types, but the customers only go
// to the cashier. All of the other clerks will go on break.

//----------------------------------------------------------------
void testCase4() {
	Thread * t;
	char * name;
	unsigned int i;

	srand(time(NULL));

	NUM_CUSTOMERS = 10;
	NUM_PP_CLERKS = 3;
	NUM_PIC_CLERKS = 2;
	NUM_APP_CLERKS = 3;
	NUM_CASHIERS = 3;
	NUM_MANAGERS = 1;

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		passportClerkLines[i] = new Clerk("Passport Clerk ", i, Clerk::PP);
		name = passportClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePassportClerk, i);
	}

	for (i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i] = new Cashier("Cashier ", i);
		name = cashierLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCashier, i);
	}

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		picClerkLines[i] = new Clerk("Pic Clerk ", i, Clerk::PIC);
		name = picClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) bePicClerk, i);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		appClerkLines[i] = new Clerk("Application Clerk ", i, Clerk::APP);
		name = appClerkLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beAppClerk, i);
	}

	for (i = 0; i < NUM_MANAGERS; ++i) {
		managers[i] = new Manager("Manager ", i);

		name = managers[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beManager, i);
	}

	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		customers[i] = new Customer(name, i, Customer::REGULAR);
		customers[i]->picDone = true;
		customers[i]->appDone = true;
		customers[i]->certified = true;
		customers[i]->money = 100;
		t->Fork((VoidFunctionPtr) cashierCustomerProcess, i);
	}
}

//----------------------------------------------------------------
// "Managers get Clerks off their break when lines get too long"
// There will be one cashier and 10 customers. Once the line accumulates
// more than 3 customers, the manager wakes up the clerk.
//----------------------------------------------------------------
void testCase5() {
	Thread * t;
	char * name;
	unsigned int i;

	srand(time(NULL));

	NUM_CUSTOMERS = 10;
	NUM_CASHIERS = 1;
	NUM_MANAGERS = 1;

	for (i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i] = new Cashier("Cashier ", i);
		cashierLines[i]->lineCount = i;
		name = cashierLines[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beCashier, i);
	}

	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		name = new char[20];
		sprintf(name, "Customer %i", i);
		t = new Thread(name);
		customers[i] = new Customer(name, i, Customer::REGULAR);
		customers[i]->picDone = true;
		customers[i]->appDone = true;
		customers[i]->certified = (i % 2 == 0 ? true : false);
		customers[i]->money = 100;
		t->Fork((VoidFunctionPtr) cashierCustomerProcess, i);
	}
	for (i = 0; i < NUM_MANAGERS; ++i) {
		managers[i] = new Manager("Manager ", i);

		name = managers[i]->name;
		t = new Thread(name);
		printf("%s on duty.\n", t->getName());
		t->Fork((VoidFunctionPtr) beManager, i);
	}
}

void PassportOffice() {
	Thread * t;
	char * name;
	int i;

	srand(time(NULL));

	testCase5();

//	printf("Please choose from the following options:\n");
//	printf(
//			"\t1. Test Case 1 -- Customers always take the shortest line, but no 2 customers ever choose the same shortest line at the same time.\n");
//	printf(
//			"\t2. Test Case 2 -- Managers only read one from one Clerk's total money received, at a time.\n");
//	printf(
//			"\t3. Test Case 3 -- Customers do not leave until they are given their passport by the Cashier. \n\tThe Cashier does not start on another customer until they know that the last Customer has left their area.\n");
//	printf(
//			"\t4. Test Case 4 -- Clerks go on break when they have no one waiting in their line.\n");
//	printf(
//			"\t5. Test Case 5 -- Managers get Clerks off their break when lines get too long.\n");
//	printf(
//			"\t6. Test Case 6 -- Total sales never suffers from a race condition.\n");
//	printf(
//			"\t7. Test Case 7 -- The behavior of Customers is proper when Senators arrive. This is before, during, and after.\n");
//	printf(
//			"\t8. Custom test case -- Specify the number of Managers, Clerks, Cashiers, and Customers.\n");
//	printf("\n");
//
//	int option = 0;
//	char c;
//	bool validinput = false;
//
//	printf("Enter a value 1-8: ");
//	scanf("%i", &option);
//
//	while (!validinput) {
//		switch (option) {
//		case 1:
//			validinput = true;
//			testCase1();
//			break;
//		case 2:
//			validinput = true;
//			printf("%i", option);
//			break;
//		case 3:
//			validinput = true;
//			testCase3();
//			break;
//		case 4:
//			validinput = true;
//			testCase4();
//			break;
//		case 5:
//			validinput = true;
//			testCase5();
//			break;
//		case 6:
//			validinput = true;
//			printf("%i", option);
//			break;
//		case 7:
//			validinput = true;
//			printf("%i", option);
//			break;
//		case 8:
//			validinput = true;
//			printf("%i", option);
//			break;
//		default:
//			printf("Invalid input. \n");
//		}
//		while ((c = getchar()) != '\n' && c != EOF)
//			;
//		if (!validinput) {
//			printf("Enter a value 1-8: ");
//			scanf("%i", &option);
//		}
//	}

//	for (i = 0; i < NUM_PP_CLERKS; ++i) {
//		passportClerkLines[i] = new Clerk("Passport Clerk ", i, Clerk::PP);
//
//		name = passportClerkLines[i]->name;
//		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
//		t->Fork((VoidFunctionPtr) bePassportClerk, i);
//	}
//
//	for (i = 0; i < NUM_CASHIERS; ++i) {
//		cashierLines[i] = new Cashier("Cashier ", i);
//
//		name = cashierLines[i]->name;
//		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
//		t->Fork((VoidFunctionPtr) beCashier, i);
//	}
//
//	for (i = 0; i < NUM_MANAGERS; ++i) {
//		managers[i] = new Manager("Manager ", i);
//
//		name = managers[i]->name;
//		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
//		t->Fork((VoidFunctionPtr) beManager, i);
//	}
//
//	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
//		picClerkLines[i] = new Clerk("Pic Clerk ", i, Clerk::PIC);
//
//		name = picClerkLines[i]->name;
//		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
//		t->Fork((VoidFunctionPtr) bePicClerk, i);
//	}
//
//	for (i = 0; i < NUM_APP_CLERKS; ++i) {
//		appClerkLines[i] = new Clerk("Application Clerk ", i, Clerk::APP);
//
//		name = appClerkLines[i]->name;
//		t = new Thread(name);
//		printf("%s on duty.\n", t->getName());
//		t->Fork((VoidFunctionPtr) beAppClerk, i);
//	}
//
//	for (i = 0; i < NUM_CUSTOMERS; ++i) {
//		name = new char[20];
//		if (i % 2 == 0) {
//			sprintf(name, "Senator %i", i);
//			t = new Thread(name);
//			customers[i] = new Customer(name, i, Customer::SENATOR);
//			printf("%s has just entered the passport office.\n", t->getName());
//			t->Fork((VoidFunctionPtr) beCustomer, i);
//		}
//		else {
//			sprintf(name, "Customer %i", i);
//			t = new Thread(name);
//			customers[i] = new Customer(name, i, Customer::REGULAR);
//			printf("%s has just entered the passport office.\n", t->getName());
//			t->Fork((VoidFunctionPtr) beCustomer, i);
//		}
//	}

	printf("\n\n");
}

