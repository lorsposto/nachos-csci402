#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	CUSTOMER, SENATOR
} customerType;

typedef enum {
	CUSTOMER, SENATOR
} customerType;

int customerIndex = 0;
int customerIndexLock = -1;

int customerCounterLock;

void picAppCustomerProcess(int customerIndex) {
	int i;
	int myLine = -1;
	int chosePic = -1;
	int lineSize = 1000;
	int bribeChance = Rand() % 5;

	 /* if (customers[customerIndex].picDone == false
			&& customers[customerIndex].appDone == false) {

		Acquire(picLineLock); 
		Acquire(appLineLock);

		 if (Rand() % 2 == 0) {
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
	else if (customers[customerIndex].appDone == false) { */
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
	/* } */

	if (myLine >= 0) {
		/* if (chosePic == 1) {

			if (customers[customerIndex].money >= 600) {

				if (bribeChance == 0) {
					picClerkLines[myLine].bribeLineCount++;
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the bribe line for PictureClerk ", 47,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);

					customers[customerIndex].money -= 500;
					picClerkLines[myLine].money += 500;
					Write("PictureClerk ", 13, ConsoleOutput);
					PrintInt(myLine);
					Write(" has received $500 from Customer ", 33,
					ConsoleOutput);
					PrintInt(customerIndex);
					Write(".\n", 2, ConsoleOutput);
					Wait(picClerkLines[myLine].bribeLineCV, picLineLock);
					picClerkLines[myLine].bribeLineCount--;
				}
				else {
					picClerkLines[myLine].regularLineCount++;
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the regular line for PictureClerk ",
							49, ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);

					Wait(picClerkLines[myLine].regularLineCV, picLineLock);
					picClerkLines[myLine].regularLineCount--;
				}

			}
			else {
				picClerkLines[myLine].regularLineCount++;
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in the regular line for PictureClerk ", 49,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				Wait(picClerkLines[myLine].regularLineCV, picLineLock);
				picClerkLines[myLine].regularLineCount--;
			}

			picClerkLines[myLine].state = BUSY;

			Release(picLineLock);
			picClerkTransaction(customerIndex, myLine);
		}
		else { */

			if (customers[customerIndex].money >= 600) {

				if (bribeChance == 0) {
					appClerkLines[myLine].bribeLineCount++;
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the bribe line for AppClerk ", 43,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);
					customers[customerIndex].money -= 500;
					appClerkLines[myLine].money += 500;
					Write("AppClerk ", 9, ConsoleOutput);
					PrintInt(myLine);
					Write(" has received $500 from Customer ", 33,
					ConsoleOutput);
					PrintInt(customerIndex);

					Wait(appClerkLines[myLine].bribeLineCV, appLineLock);
					appClerkLines[myLine].bribeLineCount--;
				}
				else {
					appClerkLines[myLine].regularLineCount++;
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in regular line for AppClerk ", 41,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);
					Wait(appClerkLines[myLine].regularLineCV, appLineLock);
					appClerkLines[myLine].regularLineCount--;
				}
			}
			else {
				appClerkLines[myLine].regularLineCount++;
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in regular line for AppClerk ", 41,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				Wait(appClerkLines[myLine].regularLineCV, appLineLock);
				appClerkLines[myLine].regularLineCount--;
			}

			appClerkLines[myLine].state = BUSY;

			Release(appLineLock);
			appClerkTransaction(customerIndex, myLine);
		/*} */
	}
}

void appClerkTransaction(int customer, int clerk) {
	/* Set the clerk's current customer */

	Acquire(appClerkLines[clerk].transactionLock);
	appClerkLines[clerk].customer = customer;
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" has given SSN ", 25, ConsoleOutput);
	PrintInt(customers[customerIndex].SSN);
	Write(" to AppClerk ", 13, ConsoleOutput);
	PrintInt(clerk);
	Write(".\n", 2, ConsoleOutput);

	Signal(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is waiting for clerk ", 22, ConsoleOutput);
	PrintInt(clerk);
	Write("to file the application.\n", 25, ConsoleOutput);
	Wait(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	Signal(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving ApplicationClerk ", 29, ConsoleOutput);
	PrintInt(clerk);
	Write("'s counter.\n", 12, ConsoleOutput);

	Release(appClerkLines[clerk].transactionLock);
}

int main() {

	int SSN;
	bool picDone;
	bool appDone;
	bool certified;
	bool gotPassport;
	bool earlybird;
	int money;
	clerkType type;

	SSN = customerIndex;
	picDone = false;
	appDone = false;
	certified = false;
	gotPassport = false;
	earlybird = false;
	money = 100;
	type = CUSTOMER;
	customerIndex++;

	int customer;

	Acquire(customerIndexLock);
	customer = Customer();
	Release(customerIndexLock);
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is entering the passport office.\n", 34, ConsoleOutput);

	if (customers[customer].type == SENATOR) {
		/* senatorSema.P(); */
		Acquire(senatorLock);
		senatorInProcess = true;
		currentSenator = customers[customer];
		Release(senatorLock);
	}

	while ((customers[customer].picDone == false
			|| customers[customer].appDone == false
			|| customers[customer].certified == false
			|| customers[customer].gotPassport == false)) {

		if (customers[customer].type != SENATOR && senatorInProcess == true) {
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
				picAppCustomerProcess(customer);
			}
			else if (customers[customer].appDone == true
					&& customers[customer].picDone == true
					&& customers[customer].certified == false
					&& customers[customer].earlybird == false) {
				passportCustomerProcess(customer);

			}
			else if (customers[customer].certified == true
					|| (customers[customer].earlybird == true
							&& customers[customer].gotPassport == false)) {
				cashierCustomerProcess(customer);
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
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving the Passport Office\n", 32, ConsoleOutput);

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
