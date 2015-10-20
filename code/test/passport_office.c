#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

void broadcastMoney();

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

int picLineLock = -1;

int customerIndex = 0;
int customerIndexLock = -1;

int passportClerkIndex = 0;
int passportClerkIndexLock = -1;

int picClerkIndex = 0;
int picClerkIndexLock = -1;

int appClerkIndex = 0;
int appClerkIndexLock = -1;

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

int Customer() {
	customers[customerIndex].SSN = customerIndex;
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
	appClerkLines[appClerkIndex].regularLineCV = CreateCondition(
			"AppClerkRegularCV", 17);
	appClerkLines[appClerkIndex].bribeLineCV = CreateCondition(
			"AppClerkBribeCV", 15);
	appClerkLines[appClerkIndex].transactionCV = CreateCondition(
			"AppClerkTransactionCV", 21);
	appClerkLines[appClerkIndex].transactionLock = CreateLock(
			"AppClerkTransactionLock", 23);
	appClerkLines[appClerkIndex].breakCV = CreateCondition("AppClerkBreakCV",
			15);
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
	picClerkLines[picClerkIndex].regularLineCV = CreateCondition(
			"PicClerkRegularCV", 17);
	picClerkLines[picClerkIndex].bribeLineCV = CreateCondition(
			"PicClerkBribeCV", 15);
	picClerkLines[picClerkIndex].transactionCV = CreateCondition(
			"PicClerkTransactionCV", 21);
	picClerkLines[picClerkIndex].transactionLock = CreateLock(
			"PicClerkTransactionLock", 23);
	picClerkLines[picClerkIndex].breakCV = CreateCondition("PicClerkBreakCV",
			15);
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
	passportClerkLines[passportClerkIndex].regularLineCV = CreateCondition(
			"PPClerkRegularCV", 16);
	passportClerkLines[passportClerkIndex].bribeLineCV = CreateCondition(
			"PPClerkBribeCV", 14);
	passportClerkLines[passportClerkIndex].transactionCV = CreateCondition(
			"PPClerkTransactionCV", 20);
	passportClerkLines[passportClerkIndex].transactionLock = CreateLock(
			"PPClerkTransactionLock", 22);
	passportClerkLines[passportClerkIndex].breakCV = CreateCondition(
			"PPClerkBreakCV", 14);
	passportClerkLines[passportClerkIndex].customer = -1;
	passportClerkLines[passportClerkIndex].money = 0;
	passportClerkIndex++;
	return passportClerkIndex;
}

int Manager(int index) {
	managers[managerIndex].index = index;
	managers[managerIndex].counter = 0;
}

void broadcastMoney() {
	int officeTotal = 0;
	int appClerkTotal = 0;
	int picClerkTotal = 0;
	int passClerkTotal = 0;
	int cashierTotal = 0;
	int i;

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		Acquire(picClerkLines[i].transactionLock);
		picClerkTotal += picClerkLines[i].money;
		Release(picClerkLines[i].transactionLock);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		Acquire(appClerkLines[i].transactionLock);
		appClerkTotal += appClerkLines[i].money;
		Release(appClerkLines[i].transactionLock);
	}

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		Acquire(passportClerkLines[i].transactionLock);
		passClerkTotal += passportClerkLines[i].money;
		Release(passportClerkLines[i].transactionLock);
	}

	for (i = 0; i < NUM_CASHIERS; ++i) {
		Acquire(cashierLines[i].transactionLock);
		cashierTotal += cashierLines[i].money;
		Release(cashierLines[i].transactionLock);
	}

	officeTotal = appClerkTotal + picClerkTotal + passClerkTotal + cashierTotal;

	/* printf("%s has counted amounts of $%i for PictureClerks\n",
			currentThread->getName(), picClerkTotal);

	printf("%s has counted amounts of $%i for ApplicationClerks\n",
			currentThread->getName(), appClerkTotal);

	printf("%s has counted amounts of $%i for PassportClerks\n",
			currentThread->getName(), passClerkTotal);

	printf("%s has counted amounts of $%i for Cashiers\n",
			currentThread->getName(), cashierTotal);

	printf("%s has counted amounts of $%i for the passport office\n",
			currentThread->getName(), officeTotal); */
}

void bePassportClerk(int passportClerkIndex) {
	int myIndex, i;
	Acquire(passportClerkIndexLock);
	myIndex = PassportClerk(passportClerkIndex);
	Release(passportClerkIndexLock);

	Write("Launching passportClerk\n", 24, ConsoleOutput);
	while (true) {
		while (passportClerkLines[myIndex].state != BREAK) {
			Acquire(passportLineLock);
			if (passportClerkLines[myIndex].bribeLineCount > 0) {
				Write("passportClerk has signaled a Customer to come to their counter.\n", 64, ConsoleOutput);
				Signal(passportClerkLines[myIndex].bribeLineCV, passportLineLock);
				passportClerkLines[myIndex].state = BUSY;
			}
			else if (passportClerkLines[myIndex].regularLineCount > 0) {
				Write("passportClerk has signaled a Customer to come to their counter.\n", 64, ConsoleOutput);
				Signal(passportClerkLines[myIndex].regularLineCV, passportLineLock);
				passportClerkLines[myIndex].state = BUSY;
			}
			else {
				passportClerkLines[myIndex].state = BREAK;
				Write("passportClerk is going on break.\n", 33, ConsoleOutput);
				Wait(passportClerkLines[myIndex].breakCV, passportLineLock);
				Write("passportClerk is coming off break.\n", 35, ConsoleOutput);
				Release(passportLineLock);
				break;
			}

			Release(passportLineLock);
			Acquire(passportClerkLines[myIndex].transactionLock);
			/* wait for Customer data */
			Wait(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);

			Write("passportClerk has received SSN from customer\n", 45, ConsoleOutput);

			if (customers[passportClerkLines[myIndex].customer].picDone
					&& customers[passportClerkLines[myIndex].customer].appDone) {

				/* TODO rand */
				/*if (rand() % 100 < 5) {*/
				if(0) {
					/* less than 5% chance that the Passport Clerk will make a mistake and send the customer
					 to the back of the line*/
					Write("passportClerk has determined that customer does not have both their application and picture completed\n",
							102, ConsoleOutput);

					passportClerkLines[myIndex].approved = false;
					Signal(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);

					Release(passportClerkLines[myIndex].transactionLock);

					/* done w this customer */
					break;
				}

				Write("passportClerk has determined that customer has both their application and picture completed\n",
						92, ConsoleOutput);

				passportClerkLines[myIndex].approved = true;

				Signal(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);
				Wait(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);

				/* Doing job, customer waiting, signal when done */

				/* Yield for a bit */
				/* for (i = 0; i < rand() % 900 + 100; ++i) { */
				for (i = 0; i < 900; ++i) {
					Yield();
				}

				/* if (rand() % 100 < 5) { */
				if (1) {
					/* less than 5% chance that the customer will leave the passport clerk too soon */
					customers[passportClerkLines[myIndex].customer].earlybird = true;
					Signal(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);
				}
				else {
					/* set application as complete */
					customers[passportClerkLines[myIndex].customer].certified = true;
					Signal(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);

					Write("passportClerk has recorded customer passport documentation\n",
							59, ConsoleOutput);
				}
			}
			else {
				passportClerkLines[myIndex].approved = false;
				Write("passportClerk has determined that customer does not have both their application and picture completed\n",
						102, ConsoleOutput);
				Signal(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);

			}

			Wait(passportClerkLines[myIndex].transactionCV, passportClerkLines[myIndex].transactionLock);

			Release(passportClerkLines[myIndex].transactionLock);
		}
	}

	Exit(0);
}

void beCashier() {

	while (true) {
		/* once they are not on break, process the line*/
		while (cashierLines[cashierIndex].state != BREAK) {
			Acquire(cashierLineLock);

			if (cashierLines[cashierIndex].lineCount > 0) {

				/*printf(
						"%s has signalled a Customer to come to their counter.\n",
						cashierLines[cashierIndex]->name);*/
				Signal(cashierLines[cashierIndex].lineCV, cashierLineLock);
				cashierLines[cashierIndex].state = BUSY;
			}
			else {
				cashierLines[cashierIndex].state = BREAK;
				/*printf("%s is going on break.\n",
						cashierLines[cashierIndex]->name);*/
				Wait(cashierLines[cashierIndex].breakCV, cashierLineLock);
				/*printf("%s is coming off break.\n",
						cashierLines[cashierIndex]->name);*/
				Release(cashierLineLock);
				break;
			}

			Acquire(cashierLines[cashierIndex].transactionLock);
			Release(cashierLineLock);

			/* wait for Customer data*/
			Wait(cashierLines[cashierIndex].transactionCV,
					cashierLines[cashierIndex].transactionLock);

			/*printf("%s has received SSN %i from %s\n",
				cashierLines[cashierIndex]->name,
				cashierLines[cashierIndex]->customer->SSN,
				cashierLines[cashierIndex]->customer->name);*/

			/* If the customer has finished everything*/
			if (customers[cashierLines[cashierIndex].customer].appDone
					&& customers[cashierLines[cashierIndex].customer].picDone
					&& customers[cashierLines[cashierIndex].customer].certified) {

				/*printf("%s has verified that %s has been certified by a PassportClerk\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);*/

				cashierLines[cashierIndex].approved = true;


				Signal(cashierLines[cashierIndex].transactionCV,
						cashierLines[cashierIndex].transactionLock);

				Wait(cashierLines[cashierIndex].transactionCV,
						cashierLines[cashierIndex].transactionLock);

	
				Signal(cashierLines[cashierIndex].transactionCV,
						cashierLines[cashierIndex].transactionLock);

				Wait(cashierLines[cashierIndex].transactionCV,
						cashierLines[cashierIndex].transactionLock);

				/*printf("%s has received the $100 from %s after certification\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);*/

				/* receive money from customer */
				cashierLines[cashierIndex].money += 100;
				customers[cashierLines[cashierIndex].customer].money -= 100;


				/* set passport as received by customer */
				customers[cashierLines[cashierIndex].customer].gotPassport = true;

				/*printf("%s has provided %s their completed passport\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);*/

				Signal(cashierLines[cashierIndex].transactionCV,
						cashierLines[cashierIndex].transactionLock);

				/*printf(
						"%s has recorded that %s has been given their completed passport\n",
						cashierLines[cashierIndex]->name,
						cashierLines[cashierIndex]->customer->name);*/
			}
			else if (customers[cashierLines[cashierIndex].customer].appDone
					&& customers[cashierLines[cashierIndex].customer].picDone
					&& customers[cashierLines[cashierIndex].customer].earlybird) {
				/*printf("%s has received the $100 from %s before certification. They are to go to the back of line\n",
				cashierLines[cashierIndex]->name, cashierLines[cashierIndex]->customer->name);*/
				cashierLines[cashierIndex].approved = false;
				customers[cashierLines[cashierIndex].customer].certified = true; /* artificially allow customer to go next time */
				/*printf("%s has gone to %s too soon. They are going to the back of the line\n",
					cashierLines[cashierIndex]->customer->name, cashierLines[cashierIndex]->name);*/
				Signal(cashierLines[cashierIndex].transactionCV,
					cashierLines[cashierIndex].transactionLock);
			}
			else {
				cashierLines[cashierIndex].approved = false;
				Signal(cashierLines[cashierIndex].transactionCV,
						cashierLines[cashierIndex].transactionLock);
			}
			Wait(cashierLines[cashierIndex].transactionCV,
					cashierLines[cashierIndex].transactionLock);
			Release(cashierLines[cashierIndex].transactionLock);
		}
	}
	Exit(0);
}

void bePicClerk() {
	int myIndex;
	bool firstTime;
	Acquire(picClerkIndexLock);
	myIndex = PicClerk(picClerkIndex);
	Release(picClerkIndexLock);

	Write("Launching picClerk\n", 19, ConsoleOutput);
	while (1) {
		while (picClerkLines[myIndex].state != BREAK) {
			Acquire(picLineLock);
			if (picClerkLines[myIndex].bribeLineCount > 0) {
				Write(
						"picClerk has signaled a Customer to come to their counter.\n",
						60, ConsoleOutput);
				Signal(picClerkLines[myIndex].bribeLineCV, picLineLock);
				picClerkLines[myIndex].state = BUSY;
			}
			else if (picClerkLines[myIndex].regularLineCount > 0) {
				Write(
						"picClerk has signaled a Customer to come to their counter.\n",
						60, ConsoleOutput);
				Signal(picClerkLines[myIndex].regularLineCV, picLineLock);
				picClerkLines[myIndex].state = BUSY;
			}
			else {
				picClerkLines[myIndex].state = BREAK;
				Write("picClerk is going on break.\n", 28, ConsoleOutput);
				Wait(picClerkLines[myIndex].breakCV, picLineLock);
				Write("picClerk is coming off break.\n", 30, ConsoleOutput);
				Release(picLineLock);
				break;
			}

			Acquire(picClerkLines[myIndex].transactionLock);
			Release(picLineLock);

			/* wait for Customer data */
			Wait(picClerkLines[myIndex].transactionCV, picClerkLines[myIndex].transactionLock);

			Write("picClerk has received SSN from customer.\n", 41,
			ConsoleOutput);

			firstTime = true;
			/* Doing job, customer waiting, signal when done */
			while (!customers[picClerkLines[myIndex].customer].picDone) {
				if (!firstTime) {
					Write(
							"picClerk has been told that customer does not like their picture\n",
							65, ConsoleOutput);
				}
				Write("picClerk has taken a picture of customer.\n", 42,
				ConsoleOutput);

				Signal(picClerkLines[myIndex].transactionCV, picClerkLines[myIndex].transactionLock);
				/* Waiting for customer to accept photo */
				Wait(picClerkLines[myIndex].transactionCV, picClerkLines[myIndex].transactionLock);

				firstTime = false;
			}

			Write(
					"picClerk has been told that customer does like their picture\n",
					61,
					ConsoleOutput);

			Signal(picClerkLines[myIndex].transactionCV, picClerkLines[myIndex].transactionLock);

			Wait(picClerkLines[myIndex].transactionCV, picClerkLines[myIndex].transactionLock);
			Release(picClerkLines[myIndex].transactionLock);
		}
	}

	Exit(0);
}

void beAppClerk() {
	int myIndex, i;
	Acquire(appClerkIndexLock);
	myIndex = AppClerk(appClerkIndex);
	Release(appClerkIndexLock);

	Write("Launching appClerk\n", 19, ConsoleOutput);
	while (1) {
		while (appClerkLines[myIndex].state != BREAK) {
			Acquire(appLineLock);
			if (appClerkLines[myIndex].bribeLineCount > 0) {
				Write(
						"appClerk has signaled a Customer to come to their counter.\n",
						60, ConsoleOutput);
				Signal(appClerkLines[myIndex].bribeLineCV, appLineLock);
				appClerkLines[myIndex].state = BUSY;
			}
			else if (appClerkLines[myIndex].regularLineCount > 0) {
				Write(
						"appClerk has signaled a Customer to come to their counter.\n",
						60, ConsoleOutput);
				Signal(appClerkLines[myIndex].regularLineCV, appLineLock);
				appClerkLines[myIndex].state = BUSY;
			}
			else {
				appClerkLines[myIndex].state = BREAK;
				Write("appClerk is going on break.\n", 28, ConsoleOutput);
				Wait(appClerkLines[myIndex].breakCV, appLineLock);
				Write("appClerk is coming off break.\n", 30, ConsoleOutput);
				Release(appLineLock);
				break;
			}

			Acquire(appClerkLines[myIndex].transactionLock);
			Release(appLineLock);

			/* wait for Customer data */
			Wait(appClerkLines[myIndex].transactionCV, appClerkLines[myIndex].transactionLock);

			Write("appClerk has received SSN from customer\n", 40,
					ConsoleOutput);

			/* Yield for a bit TODO rand */
			for (i = 0; i < 80; ++i) {
				Yield();
			}

			Write(
					"appClerk has recorded a completed application for customer\n",
					59,
					ConsoleOutput);

			/* set application as complete */
			customers[appClerkLines[myIndex].customer].appDone = true;
			Signal(appClerkLines[myIndex].transactionCV, appClerkLines[myIndex].transactionLock);
			Wait(appClerkLines[myIndex].transactionCV, appClerkLines[myIndex].transactionLock);
			Release(appClerkLines[myIndex].transactionLock);
		}
	}
	Exit(0);
}

void beCustomer() {

	int targetLock;
	int customer;
	targetLock = CreateLock("Lock", 4);
	Acquire(customerIndexLock);
	customer = Customer();
	Release(customerIndexLock);

	Write("Hi cs\n", 6, ConsoleOutput);

	if (customers[customer].type == SENATOR) {
		/* senatorSema.P(); */
		Acquire(senatorLock);
		senatorInProcess = true;
		currentSenator = customers[customer];
		Release(senatorLock);
	}

	while ((!customers[customer].picDone == true
			|| customers[customer].appDone == false
			|| customers[customer].certified == false
			|| customers[customer].gotPassport == false)) {

		if (customers[customer].type != SENATOR
				&& senatorInProcess == true) {
			Acquire(senatorLock);
			/* Write("%s is going outside the Passport Office because there is a Senator present.\n",
			 currentThread->getName()); */
			Wait(senatorCV, senatorLock);
			Release(senatorLock);
		}
		if (senatorInProcess == false
				|| (customers[customer].type == SENATOR
						&& currentSenator.SSN == customers[customer].SSN)) {

			if (customers[customer].picDone == false
					|| customers[customer].appDone == false) {
				/* picAppCustomerProcess(customerIndex); */
				Write("Yield1\n", 7, ConsoleOutput);
				Yield();
			}
			else if (customers[customer].appDone == true
					&& customers[customer].picDone == true
					&& customers[customer].certified == false
					&& customers[customer].earlybird == false) {
				/* passportCustomerProcess(customerIndex); */
				Write("Yield2\n", 7, ConsoleOutput);
				Yield();
			}
			else if (customers[customer].certified == true
					|| customers[customer].earlybird == true
							&& customers[customer].gotPassport == false) {
				/* cashierCustomerProcess(customerIndex); */
				Write("Yield3\n", 7, ConsoleOutput);
				Yield();
			}

			if (customers[customer].type != SENATOR
					&& senatorInProcess == true) {
				Acquire(senatorLock);
				Wait(senatorCV, senatorLock);
				Release(senatorLock);
			}
		}
	}

	/* Write("%s is leaving the Passport Office.\n",
	 customers[customerIndex]->name); */

	Acquire(customerCounterLock);
	customersServed++;
	Release(customerCounterLock);

	if (customers[customer].type == SENATOR && senatorInProcess == true) {
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

void picAppCustomerProcess(int customerIndex) {
	int myLine = -1;
	int chosePic = -1;
	int lineSize = 1000;
	int bribeChance = 0; /* rand() % 5; */
	int i;

	if (customers[customerIndex].picDone == false
			&& customers[customerIndex].appDone == false) {

		Acquire(picLineLock);
		Acquire(appLineLock);

		if (/* rand() % 2 */ 0 == 0) {

			Release(appLineLock);

			chosePic = 1;

			for (i = 0; i < NUM_PIC_CLERKS; ++i) {
				if (picClerkLines[i].regularLineCount
						+ picClerkLines[i].bribeLineCount < lineSize) {
					myLine = i;
					lineSize = picClerkLines[i].regularLineCount
							+ picClerkLines[i].bribeLineCount;
				}
			}
		}
		else {

			Release(picLineLock);

			chosePic = 0;

			for (i = 0; i < NUM_APP_CLERKS; ++i) {
				if (appClerkLines[i].regularLineCount
						+ appClerkLines[i].bribeLineCount < lineSize) {
					myLine = i;
					lineSize = appClerkLines[i].regularLineCount
							+ appClerkLines[i].bribeLineCount;
				}
			}
		}
	}
	else if (customers[customerIndex].picDone == false) {
		Acquire(picLineLock);

		chosePic = 1;

		for (i = 0; i < NUM_PIC_CLERKS; ++i) {
			if (picClerkLines[i].regularLineCount
					+ picClerkLines[i].bribeLineCount < lineSize) {
				myLine = i;
				lineSize = picClerkLines[i].regularLineCount
						+ picClerkLines[i].bribeLineCount;
			}
		}
	}
	else if (customers[customerIndex].appDone == false) {

		Acquire(appLineLock);

		chosePic = 0;

		for (i = 0; i < NUM_APP_CLERKS; ++i) {
			if (appClerkLines[i].regularLineCount
					+ appClerkLines[i].bribeLineCount < lineSize) {
				myLine = i;
				lineSize = appClerkLines[i].regularLineCount
						+ appClerkLines[i].bribeLineCount;
			}
		}
	}

	if (chosePic == 1) {

		if (customers[customerIndex].money >= 600) {

			if (bribeChance == 0) {
				picClerkLines[myLine].bribeLineCount++;
				/* printf("%s has gotten in bribe line for %s.\n",
						customers[customerIndex].name,
						picClerkLines[myLine].name); */
				customers[customerIndex].money -= 500;
				picClerkLines[myLine].money += 500;
				/* printf("%s has received $500 from %s.\n",
					picClerkLines[myLine].name, customers[customerIndex].name); */
				Wait(picClerkLines[myLine].bribeLineCV, picLineLock);
				picClerkLines[myLine].bribeLineCount--;
			}
			else {
				picClerkLines[myLine].regularLineCount++;
				/* printf("%s has gotten in regular line for %s.\n",
						customers[customerIndex].name,
						picClerkLines[myLine].name); */
				Wait(picClerkLines[myLine].regularLineCV, picLineLock);
				picClerkLines[myLine].regularLineCount--;
			}

		}
		else {
			picClerkLines[myLine].regularLineCount++;
			/* printf("%s has gotten in regular line for %s.\n",
					customers[customerIndex].name,
					picClerkLines[myLine].name); */
			Wait(picClerkLines[myLine].regularLineCV, picLineLock);
			picClerkLines[myLine].regularLineCount--;
		}

		picClerkLines[myLine].state = BUSY;

		Release(picLineLock);
		/* picClerkTransaction(customerIndex, myLine); */
	}
	else {

		if (customers[customerIndex].money >= 600) {

			if (bribeChance == 0) {
				appClerkLines[myLine].bribeLineCount++;
				/* printf("%s has gotten in bribe line for %s.\n",
						customers[customerIndex].name,
						appClerkLines[myLine].name); */
				customers[customerIndex].money -= 500;
				appClerkLines[myLine].money += 500;
				/* printf("%s has received $500 from %s.\n",
					appClerkLines[myLine].name, customers[customerIndex].name); */
				Wait(appClerkLines[myLine].bribeLineCV, appLineLock);
				appClerkLines[myLine].bribeLineCount--;
			}
			else {
				appClerkLines[myLine].regularLineCount++;
				/* printf("%s has gotten in regular line for %s.\n",
						customers[customerIndex]->name,
						appClerkLines[myLine]->name); */
				Wait(appClerkLines[myLine].regularLineCV, appLineLock);
				appClerkLines[myLine].regularLineCount--;
			}
		}
		else {
			appClerkLines[myLine].regularLineCount++;
			/* printf("%s has gotten in regular line for %s.\n",
					customers[customerIndex].name,
					appClerkLines[myLine].name); */
			Wait(appClerkLines[myLine].regularLineCV, appLineLock);
			appClerkLines[myLine].regularLineCount--;
		}

		appClerkLines[myLine].state = BUSY;

		Release(appLineLock);
		/* appClerkTransaction(customerIndex, myLine); */
	}
}

void passportCustomerProcess(int customerIndex) {
	int myLine = -1;
	int lineSize = 1000;
	int bribeChance = 0; /* rand() % 5; */
	int i;

	Acquire(passportLineLock);

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		if (passportClerkLines[i].regularLineCount < lineSize) {
			myLine = i;
			lineSize = passportClerkLines[i].regularLineCount;
		}
	}

	if (customers[customerIndex].money >= 600) {
		if (bribeChance == 0) {
			passportClerkLines[myLine].bribeLineCount++;
			/* printf("%s has gotten in bribe line for %s.\n",
					customers[customerIndex].name,
					passportClerkLines[myLine].name); */
			customers[customerIndex].money -= 500;
			passportClerkLines[myLine].money += 500;
			/* printf("%s has received $500 from %s.\n",
				passportClerkLines[myLine].name, customers[customerIndex]->name); */
			Wait(passportClerkLines[myLine].bribeLineCV, passportLineLock);
			passportClerkLines[myLine].bribeLineCount--;
		}
		else {
			passportClerkLines[myLine].regularLineCount++;
			/* printf("%s has gotten in regular line for %s.\n",
					customers[customerIndex].name,
					passportClerkLines[myLine].name); */
			Wait(passportClerkLines[myLine].regularLineCV, passportLineLock);
			passportClerkLines[myLine].regularLineCount--;
		}
	}
	else {
		passportClerkLines[myLine].regularLineCount++;
		/* printf("%s has gotten in regular line for %s.\n",
				customers[customerIndex].name,
				passportClerkLines[myLine].name); */
		Wait(passportClerkLines[myLine].regularLineCV, passportLineLock);
		passportClerkLines[myLine].regularLineCount--;
	}
	passportClerkLines[myLine].state = BUSY;

	Release(passportLineLock);
	/* passportClerkTransaction(customerIndex, myLine); */
}

void cashierCustomerProcess(int customerIndex) {
	int myLine = -1;
	int lineSize = 1000;
	int i;

	Acquire(cashierLineLock);

	for (i = 0; i < NUM_CASHIERS; ++i) {
		if (cashierLines[i].lineCount < lineSize) {
			myLine = i;
			lineSize = cashierLines[i].lineCount;
		}
	}

	cashierLines[myLine].lineCount++;
	/* printf("%s has gotten in regular line for %s.\n", customers[customerIndex]->name,
			cashierLines[myLine]->name); */
	Wait(cashierLines[myLine].lineCV, cashierLineLock);
	cashierLines[myLine].lineCount--;

	cashierLines[myLine].state = BUSY;

	Release(cashierLineLock);
	/* cashierTransaction(customerIndex, myLine); */
}

void picClerkTransaction(int customer, int clerk) {
	/* Set the clerk's current customer */
	Acquire(picClerkLines[clerk].transactionLock);
	picClerkLines[clerk].customer = customer;

	/* send SSN */
	/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
			customers[customer]->SSN, picClerkLines[clerk]->name);*/
	Signal(picClerkLines[clerk].transactionCV,
			picClerkLines[clerk].transactionLock);

	/* take picture

	 Wait to be shown picture
	 Take picture until he likes it*/

	while (customers[customer].picDone == false) {
		Wait(picClerkLines[clerk].transactionCV,
				picClerkLines[clerk].transactionLock);

		if (/*(rand() % 10) % 2*/ 1 == 0) {
			/*printf("%s does not like their picture from %s.\n",
					currentThread->getName(), picClerkLines[clerk]->name);*/
			Signal(picClerkLines[clerk].transactionCV,
					picClerkLines[clerk].transactionLock);
		}
		else {
			/*printf("%s does like their picture from %s.\n",
					currentThread->getName(), picClerkLines[clerk]->name);*/
			customers[customer].picDone = true;
			Signal(picClerkLines[clerk].transactionCV,
					picClerkLines[clerk].transactionLock);
		}
	}
	Wait(picClerkLines[clerk].transactionCV,
			picClerkLines[clerk].transactionLock);

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
			picClerkLines[clerk]->name);*/
	Signal(picClerkLines[clerk].transactionCV,
			picClerkLines[clerk].transactionLock);
	Release(picClerkLines[clerk].transactionLock);
}

void appClerkTransaction(int customer, int clerk) {
	/* Set the clerk's current customer */

	Acquire(appClerkLines[clerk].transactionLock);
	appClerkLines[clerk].customer = customer;
	/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
			customers[customer]->SSN, appClerkLines[clerk]->name);*/
	
	Signal(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	/*printf("%s waiting for clerk %s to file the application.\n",
	 currentThread->getName(), appClerkLines[clerk]->name);*/
	Wait(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	Signal(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	/*printf("%s is now leaving %s, releasing lock.\n", currentThread->getName(),
	 appClerkLines[clerk]->name);*/
	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
			appClerkLines[clerk]->name);*/
	Release(appClerkLines[clerk].transactionLock);
}

void passportClerkTransaction(int customer, int clerk) {
	int bribeChance;

	while (passportClerkLines[clerk].approved == false) {

		Acquire(passportClerkLines[clerk].transactionLock);
		passportClerkLines[clerk].customer = customer;

		/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
				customers[customer]->SSN, passportClerkLines[clerk]->name);*/

		Signal(passportClerkLines[clerk].transactionCV,
				passportClerkLines[clerk].transactionLock);

		Wait(passportClerkLines[clerk].transactionCV,
				passportClerkLines[clerk].transactionLock);

		if (passportClerkLines[clerk].approved == true) {
			/*stop here so we don't hop back in line*/

			/*giving info*/
			Signal(passportClerkLines[clerk].transactionCV,
					passportClerkLines[clerk].transactionLock);

			/*waiting to see if application is approved*/
			Wait(passportClerkLines[clerk].transactionCV,
					passportClerkLines[clerk].transactionLock);

			/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
					passportClerkLines[clerk]->name);*/

			Signal(passportClerkLines[clerk].transactionCV,
					passportClerkLines[clerk].transactionLock);

			passportClerkLines[clerk].approved = false;

			Release(passportClerkLines[clerk].transactionLock);

			break;
		}

		/*printf("%s has gone to %s too soon. They are going to the back of the line.\n", currentThread->getName(),
			passportClerkLines[clerk]->name);*/

		/* the 5% chance of the passport clerk "making a mistake" happened and we must get back into line*/
		Release(passportClerkLines[clerk].transactionLock);
		Acquire(passportLineLock);

		/*TODO: fix rand*/
		bribeChance = 5;

		if (bribeChance == 0) { /*decided to bribe*/
			passportClerkLines[clerk].bribeLineCount++;
			/*printf("%s has gotten in bribe line for %s.\n",
					customers[customer]->name, passportClerkLines[clerk]->name);*/
			customers[customer].money -= 500;
			passportClerkLines[clerk].money += 500;
			/*printf("%s has received $500 from %s.\n",
					passportClerkLines[clerk]->name, customers[customer]->name);*/
			Wait(passportClerkLines[clerk].bribeLineCV, passportLineLock);
			passportClerkLines[clerk].bribeLineCount--;
		}
		else { /*decided not to bribe*/
			passportClerkLines[clerk].regularLineCount++;
			/*printf("%s has gotten in regular line for %s.\n",
					customers[customer]->name, passportClerkLines[clerk]->name);*/
			Wait(passportClerkLines[clerk].regularLineCV, passportLineLock);
			passportClerkLines[clerk].regularLineCount--;
		}

		Release(passportLineLock);
	}
}

void cashierTransaction(int customer, int cashier) {
	/* Set the cashier's current customer */
	Acquire(cashierLines[cashier].transactionLock);
	cashierLines[cashier].customer = customer;

	Signal(cashierLines[cashier].transactionCV,
			cashierLines[cashier].transactionLock);

	Wait(cashierLines[cashier].transactionCV,
			cashierLines[cashier].transactionLock);

	if (cashierLines[cashier].approved == true) {
		Signal(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);

		/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
				customers[customer]->SSN, cashierLines[cashier]->name);*/
		Wait(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);

		/*printf("%s has given %s $100.\n", currentThread->getName(),
				cashierLines[cashier]->name);*/
		Signal(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);
		Wait(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);
	}

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
			cashierLines[cashier]->name);*/
	Signal(cashierLines[cashier].transactionCV,
			cashierLines[cashier].transactionLock);
	Release(cashierLines[cashier].transactionLock);
}

int main() {

	int NUM_CUSTOMERS = 10;
	int NUM_SENATORS = 0;
	int NUM_PIC_CLERKS = 0;
	int NUM_APP_CLERKS = 0;
	int NUM_PP_CLERKS = 0;
	int NUM_CASHIERS = 0;
	int NUM_MANAGERS = 0;

	int sen = NUM_SENATORS;

	int i;
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

	picClerkIndexLock = CreateLock("picClerkIndexLock", 17);
	appClerkIndexLock = CreateLock("appClerkIndexLock", 17);
	passportClerkIndexLock = CreateLock("ppClerkIndexLock", 16);
	customerIndexLock = CreateLock("customerIndexLock", 17);

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		Fork(bePassportClerk);
	}

	/*for (i = 0; i < NUM_CASHIERS; ++i) {
	 struct Cashier c;
	 cashierLines[i] = c;
	 Fork(beCashier);
	 }*/

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		Fork(bePicClerk);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		Fork(beAppClerk);
	}

	NUM_CUSTOMERS += NUM_SENATORS;
	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		if (sen > 0) {
			Fork(beCustomer);
			sen--;
		}
		else {
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
