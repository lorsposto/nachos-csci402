#include "syscall.h"

#define NULL 0
typedef enum { false, true } bool;

int NUM_CUSTOMERS = 0;
int NUM_SENATORS = 0;
int NUM_PIC_CLERKS = 0;
int NUM_APP_CLERKS = 0;
int NUM_PP_CLERKS = 0;
int NUM_CASHIERS = 0;
int NUM_MANAGERS = 0;
int customersServed = 0;

bool senatorInProcess = false;
struct Customer currentSenator;

struct Clerk picClerkLines[100];
struct Clerk appClerkLines[100];
struct Clerk passportClerkLines[100];
struct Cashier cashierLines[100];

struct Manager managers[100];

struct Customer customers[100];

typedef enum {
	CUSTOMER, SENATOR
} customerType;

typedef enum {
	APP, PIC, PP
} clerkType;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int customerIndex = 0;
int passportClerkIndex = 0;
int picClerkIndex = 0;
int appClerkIndex = 0;

int picLineLock;
int appLineLock;
int passportLineLock;
int cashierLineLock;
int customerCounterLock;
int managerCounterLock;
int senatorLock;
/* Semaphore senatorSema; */
int senatorCV;
int managerIndex = 0;
int cashierIndex = 0;


struct Customer {
	int SSN;
	bool picDone;
	bool appDone;
	bool certified;
	bool gotPassport;
	bool earlybird;
	int money;
	clerkType type;
};

struct Clerk {
	int index;
	bool approved;
	int bribeLineCount;
	int regularLineCount;
	clerkType type;
	clerkState state;
	int regularLineCV;
	int bribeLineCV;
	int transactionCV;
	int transactionLock;

	int breakCV;

	int customer;
	int money;
};

struct Manager {
	int index;
	int counter;
};

struct Cashier {
	int index;
	bool approved;
	int lineCount;
	int money;
	clerkState state;
	int lineCV;
	int transactionCV;
	int transactionLock;

	int breakCV;

	int customer;
};

int Customer(int ssn) {
	customers[customerIndex].SSN = ssn;
	customers[customerIndex].picDone = false;
	customers[customerIndex].appDone = false;
	customers[customerIndex].certified = false;
	customers[customerIndex].gotPassport = false;
	customers[customerIndex].earlybird = false;
	customers[customerIndex].money = 100;
	customers[customerIndex].type = CUSTOMER;
	customerIndex++;
	return customerIndex;
}

int Senator(int ssn) {
	customers[customerIndex].SSN = ssn;
	customers[customerIndex].picDone = false;
	customers[customerIndex].appDone = false;
	customers[customerIndex].certified = false;
	customers[customerIndex].gotPassport = false;
	customers[customerIndex].earlybird = false;
	customers[customerIndex].money = 100;
	customers[customerIndex].type = SENATOR;
	customerIndex++;
	return customerIndex;
}

int AppClerk(int index) {
	appClerkLines[appClerkIndex].index = index;
	appClerkLines[appClerkIndex].approved = false;
	appClerkLines[appClerkIndex].bribeLineCount = 0;
	appClerkLines[appClerkIndex].regularLineCount = 0;
	appClerkLines[appClerkIndex].type = APP;
	appClerkLines[appClerkIndex].state = AVAILABLE;
	appClerkLines[appClerkIndex].regularLineCV = CreateCondition("AppClerkRegularCV", 17);
	appClerkLines[appClerkIndex].bribeLineCV = CreateCondition("AppClerkBribeCV", 15);
	appClerkLines[appClerkIndex].transactionCV = CreateCondition("AppClerkTransactionCV", 21);
	appClerkLines[appClerkIndex].transactionLock = CreateLock("AppClerkTransactionLock", 23);
	appClerkLines[appClerkIndex].breakCV = CreateCondition("AppClerkBreakCV", 15);
	appClerkLines[appClerkIndex].customer = -1;
	appClerkLines[appClerkIndex].money = 0;
	appClerkIndex++;
	return appClerkIndex;
}

int PicClerk(int index) {
	picClerkLines[picClerkIndex].index = index;
	picClerkLines[picClerkIndex].approved = false;
	picClerkLines[picClerkIndex].bribeLineCount = 0;
	picClerkLines[picClerkIndex].regularLineCount = 0;
	picClerkLines[picClerkIndex].type = PIC;
	picClerkLines[picClerkIndex].state = AVAILABLE;
	picClerkLines[picClerkIndex].regularLineCV = CreateCondition("PicClerkRegularCV", 17);
	picClerkLines[picClerkIndex].bribeLineCV = CreateCondition("PicClerkBribeCV", 15);
	picClerkLines[picClerkIndex].transactionCV = CreateCondition("PicClerkTransactionCV", 21);
	picClerkLines[picClerkIndex].transactionLock = CreateLock("PicClerkTransactionLock", 23);
	picClerkLines[picClerkIndex].breakCV = CreateCondition("PicClerkBreakCV", 15);
	picClerkLines[picClerkIndex].customer = -1;
	picClerkLines[picClerkIndex].money = 0;
	picClerkIndex++;
	return picClerkIndex;
}

int PassportClerk(int index) {
	passportClerkLines[passportClerkIndex].index = index;
	passportClerkLines[passportClerkIndex].approved = false;
	passportClerkLines[passportClerkIndex].bribeLineCount = 0;
	passportClerkLines[passportClerkIndex].regularLineCount = 0;
	passportClerkLines[passportClerkIndex].type = PP;
	passportClerkLines[passportClerkIndex].state = AVAILABLE;
	passportClerkLines[passportClerkIndex].regularLineCV = CreateCondition("PPClerkRegularCV", 16);
	passportClerkLines[passportClerkIndex].bribeLineCV = CreateCondition("PPClerkBribeCV", 14);
	passportClerkLines[passportClerkIndex].transactionCV = CreateCondition("PPClerkTransactionCV", 20);
	passportClerkLines[passportClerkIndex].transactionLock = CreateLock("PPClerkTransactionLock", 22);
	passportClerkLines[passportClerkIndex].breakCV = CreateCondition("PPClerkBreakCV", 14);
	passportClerkLines[passportClerkIndex].customer = -1;
	passportClerkLines[passportClerkIndex].money = 0;
	passportClerkIndex++;
	return passportClerkIndex;
}

int Manager(int index) {
	managers[managerIndex].index = index;
	managers[managerIndex].counter = 0;
}

void bePassportClerk(int passportClerkIndex) {
	Write("Hi pp\n", 6, ConsoleOutput);
	Exit(0);
}

void beCashier(int cashierIndex) {
	Exit(0);
}

void bePicClerk(int picClerkIndex) {
	Write("Hi pi\n", 6, ConsoleOutput);
	Exit(0);
}

void beAppClerk(int appClerkIndex) {
	Write("Hi ap\n", 6, ConsoleOutput);
	Exit(0);
}

void beCustomer(int customerIndex) {

	int targetLock = CreateLock("Lock", 4);

	Write("Hi cs\n", 6, ConsoleOutput);

	if (customers[customerIndex].type == SENATOR) {
		/* senatorSema.P(); */
		Acquire(senatorLock);
		senatorInProcess = true;
		currentSenator = customers[customerIndex];
		Release(senatorLock);
	}

	while ((!customers[customerIndex].picDone == true
			|| customers[customerIndex].appDone == false
			|| customers[customerIndex].certified == false
			|| customers[customerIndex].gotPassport == false)) {

		if (customers[customerIndex].type != SENATOR
				&& senatorInProcess == true) {
			Acquire(senatorLock);
			/* printf("%s is going outside the Passport Office because there is a Senator present.\n",
					currentThread->getName()); */
			Wait(senatorCV, senatorLock);
			Release(senatorLock);
		}
		if (senatorInProcess == false ||
			(customers[customerIndex].type == SENATOR && currentSenator.SSN == customers[customerIndex].SSN)) {

			if (customers[customerIndex].picDone == false
					|| customers[customerIndex].appDone == false) {
				/* picAppCustomerProcess(customerIndex); */
			}
			else if (customers[customerIndex].appDone == true
					&& customers[customerIndex].picDone == true
					&& customers[customerIndex].certified == false
					&& customers[customerIndex].earlybird == false) {
				/* passportCustomerProcess(customerIndex); */
			}
			else if (customers[customerIndex].certified == true
					|| customers[customerIndex].earlybird == true
					&& customers[customerIndex].gotPassport == false) {
				/* cashierCustomerProcess(customerIndex); */
			}

			if (customers[customerIndex].type != SENATOR
					&& senatorInProcess == true) {
				Acquire(senatorLock);
				Wait(senatorCV, senatorLock);
				Release(senatorLock);
			}
		}
	}

	/* printf("%s is leaving the Passport Office.\n",
			customers[customerIndex]->name); */

	Acquire(customerCounterLock);
	customersServed++;
	Release(customerCounterLock);

	if (customers[customerIndex].type == SENATOR
			&& senatorInProcess == true) {
		Acquire(senatorLock);
		/* currentSenator = NULL; */
		senatorInProcess = false;
		Broadcast(senatorCV, senatorLock);
		Release(senatorLock);
		/* senatorSema.V(); */
	}

	Exit(0);
}

void beManager() {
	int i;
	int j;
	int myIndex;
	bool allOnBreak;

	Acquire(managerCounterLock);
	myIndex = Manager(managerIndex);
	Release(managerCounterLock);

	while (customersServed < NUM_CUSTOMERS) {
		
		if (managers[myIndex].counter % 2 == 0) {
			/* broadcastMoney();*/
		}

		/*checks if all clerks are on break. Because the manager terminates when all customers are served,
		if there are still customers to be served and all clerks of a type are on break we know there are
		<3 people in line who must be served*/
		allOnBreak = true;
		managers[myIndex].counter++;

		Acquire(picLineLock);
		for (i = 0; i < picClerkIndex; ++i) {

			if (picClerkLines[i].state == BREAK
					&& (picClerkLines[i].bribeLineCount
							+ picClerkLines[i].regularLineCount > 2
							|| senatorInProcess == true)) {
				picClerkLines[i].state = AVAILABLE;
				
				Signal(picClerkLines[i].breakCV, picLineLock);

				/*CHANGE TO NOT BE PRINTF
				printf("%s has woken up a PictureClerk\n",
						managers[index]->name);*/

				allOnBreak = false;
			} else {
				if(picClerkLines[i].state != BREAK)
					allOnBreak = false;
			}
		}
		/*all picture clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if(allOnBreak == true) {
			for (i = 0; i < picClerkIndex; ++i) {
				if(picClerkLines[i].bribeLineCount
						+ picClerkLines[i].regularLineCount > 0) {
					picClerkLines[i].state = AVAILABLE;
					
					Signal(picClerkLines[i].breakCV, picLineLock);
					
					/*CHANGE TO NOT BE PRINTF 
					printf("%s has woken up a PictureClerk\n",
						managers[index]->name);*/
				}
			}
		}

		allOnBreak = true;

		Release(picLineLock);

		Acquire(appLineLock);

		for (i = 0; i < appClerkIndex; ++i) {

			if (appClerkLines[i].state == BREAK
					&& (appClerkLines[i].bribeLineCount
							+ appClerkLines[i].regularLineCount > 2
							|| senatorInProcess == true)) {
				appClerkLines[i].state = AVAILABLE;
				
				Signal(appClerkLines[i].breakCV, appLineLock);

				/*CHANGE TO NOT BE PRINTF 
				printf("%s has woken up an ApplicationClerk\n",
						managers[index]->name);*/
				allOnBreak = false;
			} else {
				if(appClerkLines[i].state != BREAK)
					allOnBreak = false;
			}
		}
		/*all app clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if(allOnBreak == true) {
			for (i = 0; i < appClerkIndex; ++i) {
				if(appClerkLines[i].bribeLineCount
						+ appClerkLines[i].regularLineCount > 0) {
					appClerkLines[i].state = AVAILABLE;
					
					Signal(appClerkLines[i].breakCV, appLineLock);
					/*FIX printf("%s has woken up an ApplicationClerk\n",
						managers[index]->name);*/
				}
			}
		}

		allOnBreak = true;

		Release(appLineLock);
		
		Acquire(passportLineLock);

		for (i = 0; i < passportClerkIndex; ++i) {

			if (passportClerkLines[i].state == BREAK
					&& (passportClerkLines[i].bribeLineCount
							+ passportClerkLines[i].regularLineCount > 2
							|| senatorInProcess)) {
				passportClerkLines[i].state = AVAILABLE;
				Signal(passportClerkLines[i].breakCV, passportLineLock);
				/*FIX printf("%s has woken up a PassportClerk\n",
						managers[index]->name);*/
				allOnBreak = false;
			} else {
				if(passportClerkLines[i].state != BREAK)
					allOnBreak = false;
			}
		}

		/*all passport clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if(allOnBreak == true) {
			for (i = 0; i < passportClerkIndex; ++i) {
				if(passportClerkLines[i].bribeLineCount
						+ passportClerkLines[i].regularLineCount > 0) {
					passportClerkLines[i].state = AVAILABLE;
					Signal(passportClerkLines[i].breakCV, passportLineLock);
					/*FIX printf("%s has woken up an PassportClerk\n",
						managers[index]->name);*/
				}
			}
		}

		allOnBreak = true;

		Release(passportLineLock);
		Acquire(cashierLineLock);

		for (i = 0; i < cashierIndex; ++i) {

			if (cashierLines[i].state == BREAK
					&& (cashierLines[i].lineCount > 2 || senatorInProcess == true)) {
				cashierLines[i].state = AVAILABLE;
				Signal(cashierLines[i].breakCV, cashierLineLock);
				/*FIX printf("%s has woken up a Cashier\n", managers[index]->name);*/
				allOnBreak = false;
			} else {
				if(cashierLines[i].state != BREAK)
					allOnBreak = false;
			}
		}

		/*all cashier clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if(allOnBreak == true) {
			for (j = 0; j < cashierIndex; ++j) {
				if(cashierLines[j].lineCount > 0) {
					cashierLines[j].state = AVAILABLE;
					Signal(cashierLines[j].breakCV, cashierLineLock);
					/*printf("%s has woken up a Cashier\n",
						managers[index]->name);*/
				}
			}
		}

		allOnBreak = true;
		Release(cashierLineLock);

		/*fix to use RAND when we make a syscall for that*/
		for (i = 0; i < 250; ++i) {
			Yield();
		}
	}


	Exit(0);
}

int main() {

	int NUM_CUSTOMERS = 10;
	int NUM_SENATORS = 0;
	int NUM_PIC_CLERKS = 1;
	int NUM_APP_CLERKS = 1;
	int NUM_PP_CLERKS = 1;
	int NUM_CASHIERS = 0;
	int NUM_MANAGERS = 0;

	int sen = NUM_SENATORS;

	unsigned int i;
	int option;
	char c;
	bool validinput;

	picLineLock = CreateLock("Pic Line Lock", 13);
	appLineLock = CreateLock("App Line Lock", 13);
	passportLineLock = CreateLock("Passport Line Lock", 18);
	cashierLineLock = CreateLock("Cashier Line Lock", 17);
	customerCounterLock = CreateLock("Customer Counter Lock", 21);
	managerCounterLock = CreateLock("Manager Counter Lock", 20);

	senatorLock = CreateLock("Senator Lock", 12);
	/* Semaphore senatorSema("Senator Semaphore", 1); */
	senatorCV = CreateCondition("Senator CV", 10);

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		PassportClerk(passportClerkIndex);
		Fork(bePassportClerk);
	}

	/*for (i = 0; i < NUM_CASHIERS; ++i) {
		struct Cashier c;
		cashierLines[i] = c;
		Fork(beCashier);
	}*/

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		PicClerk(picClerkIndex);
		Fork(bePicClerk);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		AppClerk(appClerkIndex);
		Fork(beAppClerk);
	}

	NUM_CUSTOMERS += NUM_SENATORS;
	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		if (sen > 0) {
			Senator(customerIndex);
			Fork(beCustomer);
			sen--;
		}
		else {
			Customer(customerIndex);
			Fork(beCustomer);
		}
	}

	for (i = 0; i < NUM_MANAGERS; ++i) {
		Fork(beManager);
	}

	/* TODO: figure out how to get user input */
	/*option = 0;
	validinput = false;

	Write("Enter a value 1-8: ", 20, ConsoleOutput);

	Read(option, 1, ConsoleInput);

	while (!validinput) {
		switch (option) {
			case 1:
				validinput = true;
				break;
			case 2:
				validinput = true;
				break;
			case 3:
				validinput = true;
				break;
			case 4:
				validinput = true;
				break;
			case 5:
				validinput = true;
				break;
			case 6:
				validinput = true;
				break;
			case 7:
				validinput = true;
				break;
			case 8:
				validinput = true;
				break;
			default:
				Write("Invalid input. \n", 16, ConsoleOutput);
		}
	}

	Write("\n\n", 2, ConsoleOutput);*/
}
