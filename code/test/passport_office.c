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

int cashierIndex = 0;
int cashierIndexLock = -1;

int managerIndex = 0;

int picLineLock;
int appLineLock;
int passportLineLock;
int cashierLineLock;
int customerCounterLock;
int managerCounterLock;
int cashierCounterLock;
int senatorLock;

int senatorCV;

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
	return customerIndex - 1;
}

int Senator() {
	customers[customerIndex].SSN = customerIndex;
	customers[customerIndex].picDone = false;
	customers[customerIndex].appDone = false;
	customers[customerIndex].certified = false;
	customers[customerIndex].gotPassport = false;
	customers[customerIndex].earlybird = false;
	customers[customerIndex].money = 100;
	customers[customerIndex].type = SENATOR;
	customerIndex++;
	return customerIndex - 1;
}

int AppClerk() {
	appClerkLines[appClerkIndex].index = appClerkIndex;
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
	return appClerkIndex - 1;
}

int PicClerk() {
	picClerkLines[picClerkIndex].index = picClerkIndex;
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
	return picClerkIndex - 1;
}

int PassportClerk() {
	passportClerkLines[passportClerkIndex].index = passportClerkIndex;
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
	return passportClerkIndex - 1;
}

int Cashier() {
	cashierLines[cashierIndex].index = cashierIndex;
	cashierLines[cashierIndex].approved = false;
	cashierLines[cashierIndex].lineCount = 0;
	cashierLines[cashierIndex].money = 0;
	cashierLines[cashierIndex].state = AVAILABLE;
	cashierLines[cashierIndex].lineCV = CreateCondition("CashierLineCV", 13);
	cashierLines[cashierIndex].transactionCV = CreateCondition(
			"CashierTransactionCV", 20);
	cashierLines[cashierIndex].transactionLock = CreateLock(
			"CashierTransactionLock", 22);
	cashierLines[cashierIndex].breakCV = CreateCondition("CashierBreakCV", 14);
	cashierLines[cashierIndex].customer = -1;
	cashierIndex++;
	return cashierIndex - 1;
}

int Manager() {
	managers[managerIndex].index = managerIndex;
	managers[managerIndex].counter = 0;
	managerIndex++;
	return managerIndex - 1;
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

	Write("Manager ", 8, ConsoleOutput);
	Write(" has counted amounts of $", 25, ConsoleOutput);
	PrintInt(picClerkTotal);
	Write(" for PictureClerks\n", 19, ConsoleOutput);

	Write("Manager ", 8, ConsoleOutput);
	Write(" has counted amounts of $", 25, ConsoleOutput);
	PrintInt(appClerkTotal);
	Write(" for ApplicationClerks\n", 23, ConsoleOutput);

	Write("Manager ", 8, ConsoleOutput);
	Write(" has counted amounts of $", 25, ConsoleOutput);
	PrintInt(passClerkTotal);
	Write(" for PassportClerks\n", 20, ConsoleOutput);

	Write("Manager ", 8, ConsoleOutput);
	Write(" has counted amounts of $", 25, ConsoleOutput);
	PrintInt(cashierTotal);
	Write(" for Cashiers\n", 14, ConsoleOutput);

	Write("Manager ", 8, ConsoleOutput);
	Write(" has counted amounts of $", 25, ConsoleOutput);
	PrintInt(officeTotal);
	Write(" for the passport office\n", 25, ConsoleOutput);
}

void bePassportClerk() {
	int myIndex;
	int i;
	Acquire(passportClerkIndexLock);
	myIndex = PassportClerk(passportClerkIndex);
	Release(passportClerkIndexLock);

	while (true) {
		while (passportClerkLines[myIndex].state != BREAK) {
			Acquire(passportLineLock);
			if (passportClerkLines[myIndex].bribeLineCount > 0) {
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(passportClerkLines[myIndex].bribeLineCV,
						passportLineLock);
				passportClerkLines[myIndex].state = BUSY;
			}
			else if (passportClerkLines[myIndex].regularLineCount > 0) {
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(passportClerkLines[myIndex].regularLineCV,
						passportLineLock);
				passportClerkLines[myIndex].state = BUSY;
			}
			else {
				passportClerkLines[myIndex].state = BREAK;
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(passportClerkLines[myIndex].breakCV, passportLineLock);
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(passportLineLock);
				break;
			}

			Release(passportLineLock);
			Acquire(passportClerkLines[myIndex].transactionLock);
			/* wait for Customer data */
			Wait(passportClerkLines[myIndex].transactionCV,
					passportClerkLines[myIndex].transactionLock);

			Write("PassportClerk ", 14, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(customers[passportClerkLines[myIndex].customer].SSN);
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(passportClerkLines[myIndex].customer);
			Write(" .\n", 3, ConsoleOutput);

			if (customers[passportClerkLines[myIndex].customer].picDone
					&& customers[passportClerkLines[myIndex].customer].appDone) {

				if (Rand() % 100 < 5) {
					/* less than 5% chance that the Passport Clerk will make a mistake and send the customer
					 to the back of the line*/
					Write("PassportClerk ", 14, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has determined that Customer ", 30, ConsoleOutput);
					PrintInt(passportClerkLines[myIndex].customer);
					Write(
							" does not have both their application and picture completed\n",
							60, ConsoleOutput);

					passportClerkLines[myIndex].approved = false;
					Signal(passportClerkLines[myIndex].transactionCV,
							passportClerkLines[myIndex].transactionLock);

					Release(passportClerkLines[myIndex].transactionLock);

					/* done w this customer */
					break;
				}

				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has determined that Customer ", 30, ConsoleOutput);
				PrintInt(passportClerkLines[myIndex].customer);
				Write(
						" does have both their application and picture completed\n",
						56, ConsoleOutput);

				passportClerkLines[myIndex].approved = true;

				Signal(passportClerkLines[myIndex].transactionCV,
						passportClerkLines[myIndex].transactionLock);
				Wait(passportClerkLines[myIndex].transactionCV,
						passportClerkLines[myIndex].transactionLock);

				/* Doing job, customer waiting, signal when done */

				for (i = 0; i < Rand() % 900 + 100; ++i) {
					Yield();
				}

				if (Rand() % 100 < 5) {
					/* less than 5% chance that the customer will leave the passport clerk too soon */
					customers[passportClerkLines[myIndex].customer].earlybird =
							true;
					Signal(passportClerkLines[myIndex].transactionCV,
							passportClerkLines[myIndex].transactionLock);
				}
				else {
					/* set application as complete */
					customers[passportClerkLines[myIndex].customer].certified =
							true;
					Signal(passportClerkLines[myIndex].transactionCV,
							passportClerkLines[myIndex].transactionLock);

					Write("PassportClerk ", 14, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has recorded Customer ", 23, ConsoleOutput);
					PrintInt(passportClerkLines[myIndex].customer);
					Write(" passport documentation\n", 25, ConsoleOutput);
				}
			}
			else {
				passportClerkLines[myIndex].approved = false;

				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has determined that Customer ", 23, ConsoleOutput);
				PrintInt(passportClerkLines[myIndex].customer);
				Write(
						" does not have both their application and picture completed\n",
						60, ConsoleOutput);

				Signal(passportClerkLines[myIndex].transactionCV,
						passportClerkLines[myIndex].transactionLock);

			}

			Wait(passportClerkLines[myIndex].transactionCV,
					passportClerkLines[myIndex].transactionLock);

			Release(passportClerkLines[myIndex].transactionLock);
		}
	}

	Exit(0);
}

void beCashier() {
	int myIndex;

	Acquire(cashierIndexLock);
	myIndex = Cashier(cashierIndex);
	Release(passportClerkIndexLock);

	while (true) {
		/* once they are not on break, process the line*/
		while (cashierLines[myIndex].state != BREAK) {
			Acquire(cashierLineLock);

			if (cashierLines[myIndex].lineCount > 0) {

				/*printf(
				 "%s has signalled a Customer to come to their counter.\n",
				 cashierLines[cashierIndex]->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);

				Signal(cashierLines[myIndex].lineCV, cashierLineLock);
				cashierLines[myIndex].state = BUSY;
			}
			else {
				cashierLines[myIndex].state = BREAK;
				/*printf("%s is going on break.\n",
				 cashierLines[cashierIndex]->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);

				Wait(cashierLines[myIndex].breakCV, cashierLineLock);
				/*printf("%s is coming off break.\n",
				 cashierLines[cashierIndex]->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);

				Release(cashierLineLock);
				break;
			}

			Acquire(cashierLines[myIndex].transactionLock);
			Release(cashierLineLock);

			/* wait for Customer data*/
			Wait(cashierLines[myIndex].transactionCV,
					cashierLines[myIndex].transactionLock);

			/*printf("%s has received SSN %i from %s\n",
			 cashierLines[cashierIndex]->name,
			 cashierLines[cashierIndex]->customer->SSN,
			 cashierLines[cashierIndex]->customer->name);*/

			Write("Cashier ", 8, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(customers[cashierLines[myIndex].customer].SSN);
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(cashierLines[myIndex].customer);
			Write(" .\n", 3, ConsoleOutput);

			/* If the customer has finished everything*/
			if (customers[cashierLines[myIndex].customer].appDone
					&& customers[cashierLines[myIndex].customer].picDone
					&& customers[cashierLines[myIndex].customer].certified) {

				/*printf("%s has verified that %s has been certified by a PassportClerk\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has verified that Customer ", 18, ConsoleOutput);
				PrintInt(cashierLines[myIndex].customer);
				Write(" has been certified by a PassportClerk.\n", 40,
				ConsoleOutput);

				cashierLines[myIndex].approved = true;

				Signal(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);

				Wait(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);

				Signal(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);

				Wait(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);

				/*printf("%s has received the $100 from %s after certification\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has received the $100 from Customer ", 37,
				ConsoleOutput);
				PrintInt(cashierLines[myIndex].customer);
				Write(" after certification.\n", 21, ConsoleOutput);

				/* receive money from customer */
				cashierLines[myIndex].money += 100;
				customers[cashierLines[myIndex].customer].money -= 100;

				/* set passport as received by customer */
				customers[cashierLines[myIndex].customer].gotPassport = true;

				/*printf("%s has provided %s their completed passport\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has provided Customer ", 23, ConsoleOutput);
				PrintInt(cashierLines[myIndex].customer);
				Write(" their completed passport.\n", 26, ConsoleOutput);

				Signal(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);

				/*printf(
				 "%s has recorded that %s has been given their completed passport\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/

				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has recorded that Customer ", 28, ConsoleOutput);
				PrintInt(cashierLines[myIndex].customer);
				Write(" has been given their completed passport.\n", 42,
				ConsoleOutput);
			}
			else if (customers[cashierLines[myIndex].customer].appDone
					&& customers[cashierLines[myIndex].customer].picDone
					&& customers[cashierLines[myIndex].customer].earlybird) {
				/*printf("%s has received the $100 from %s before certification. They are to go to the back of line\n",
				 cashierLines[cashierIndex]->name, cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has received the $100 from Customer ", 37,
				ConsoleOutput);
				PrintInt(cashierLines[myIndex].customer);
				Write(" before certification.\n", 22, ConsoleOutput);

				cashierLines[myIndex].approved = false;
				customers[cashierLines[myIndex].customer].certified = true; /* artificially allow customer to go next time */
				/*printf("%s has gone to %s too soon. They are going to the back of the line\n",
				 cashierLines[cashierIndex]->customer->name, cashierLines[cashierIndex]->name);*/
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(cashierLines[myIndex].customer);
				Write(" has gone to Cashier ", 20, ConsoleOutput);
				PrintInt(myIndex);
				Write(" too soon. They are going to the back of the line.\n",
						51, ConsoleOutput);

				Signal(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);
			}
			else {
				cashierLines[myIndex].approved = false;
				Signal(cashierLines[myIndex].transactionCV,
						cashierLines[myIndex].transactionLock);
			}
			Wait(cashierLines[myIndex].transactionCV,
					cashierLines[myIndex].transactionLock);
			Release(cashierLines[myIndex].transactionLock);
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

	while (1) {
		while (picClerkLines[myIndex].state != BREAK) {
			Acquire(picLineLock);
			if (picClerkLines[myIndex].bribeLineCount > 0) {
				Write("PictureClerk ", 13, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(picClerkLines[myIndex].bribeLineCV, picLineLock);
				picClerkLines[myIndex].state = BUSY;
			}
			else if (picClerkLines[myIndex].regularLineCount > 0) {
				Write("PictureClerk ", 13, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(picClerkLines[myIndex].regularLineCV, picLineLock);
				picClerkLines[myIndex].state = BUSY;
			}
			else {
				picClerkLines[myIndex].state = BREAK;
				Write("PictureClerk ", 13, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(picClerkLines[myIndex].breakCV, picLineLock);
				Write("PictureClerk ", 13, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(picLineLock);
				break;
			}

			Acquire(picClerkLines[myIndex].transactionLock);
			Release(picLineLock);

			/* wait for Customer data */
			Wait(picClerkLines[myIndex].transactionCV,
					picClerkLines[myIndex].transactionLock);

			Write("PictureClerk ", 13, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(customers[picClerkLines[myIndex].customer].SSN);
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(picClerkLines[myIndex].customer);
			Write(" .\n", 3, ConsoleOutput);

			firstTime = true;
			/* Doing job, customer waiting, signal when done */
			while (!customers[picClerkLines[myIndex].customer].picDone) {
				if (!firstTime) {
					Write("PictureClerk ", 13, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has been told that Customer ", 29, ConsoleOutput);
					PrintInt(picClerkLines[myIndex].customer);
					Write(" does not like their picture.\n", 30, ConsoleOutput);
				}
				Write("PictureClerk ", 13, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has taken a picture of Customer ", 33, ConsoleOutput);
				PrintInt(picClerkLines[myIndex].customer);
				Write(".\n", 2, ConsoleOutput);

				Signal(picClerkLines[myIndex].transactionCV,
						picClerkLines[myIndex].transactionLock);
				/* Waiting for customer to accept photo */
				Wait(picClerkLines[myIndex].transactionCV,
						picClerkLines[myIndex].transactionLock);

				firstTime = false;
			}

			Write("PictureClerk ", 13, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has been told that Customer ", 29, ConsoleOutput);
			PrintInt(picClerkLines[myIndex].customer);
			Write(" does like their picture.\n", 26, ConsoleOutput);

			Signal(picClerkLines[myIndex].transactionCV,
					picClerkLines[myIndex].transactionLock);

			Wait(picClerkLines[myIndex].transactionCV,
					picClerkLines[myIndex].transactionLock);
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

	while (1) {
		while (appClerkLines[myIndex].state != BREAK) {
			Acquire(appLineLock);
			if (appClerkLines[myIndex].bribeLineCount > 0) {
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(appClerkLines[myIndex].bribeLineCV, appLineLock);
				appClerkLines[myIndex].state = BUSY;
			}
			else if (appClerkLines[myIndex].regularLineCount > 0) {
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(appClerkLines[myIndex].regularLineCV, appLineLock);
				appClerkLines[myIndex].state = BUSY;
			}
			else {
				appClerkLines[myIndex].state = BREAK;
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(appClerkLines[myIndex].breakCV, appLineLock);
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(appLineLock);
				break;
			}

			Acquire(appClerkLines[myIndex].transactionLock);
			Release(appLineLock);

			/* wait for Customer data */
			Wait(appClerkLines[myIndex].transactionCV,
					appClerkLines[myIndex].transactionLock);

			Write("ApplicationClerk ", 17, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(customers[appClerkLines[myIndex].customer].SSN);
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(appClerkLines[myIndex].customer);
			Write(" .\n", 3, ConsoleOutput);

			/* Yield for a bit TODO rand */
			for (i = 0; i < 80; ++i) {
				Yield();
			}

			Write("ApplicationClerk ", 17, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has recorded a complete application for Customer ", 50,
			ConsoleOutput);
			PrintInt(appClerkLines[myIndex].customer);
			Write(".\n", 2, ConsoleOutput);

			/* set application as complete */
			customers[appClerkLines[myIndex].customer].appDone = true;
			Signal(appClerkLines[myIndex].transactionCV,
					appClerkLines[myIndex].transactionLock);
			Wait(appClerkLines[myIndex].transactionCV,
					appClerkLines[myIndex].transactionLock);
			Release(appClerkLines[myIndex].transactionLock);
		}
	}
	Exit(0);
}

void beCustomer() {
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
			broadcastMoney();
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

				/*printf("%s has woken up a PictureClerk\n",
				 managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a PictureClerk.\n", 29, ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (picClerkLines[i].state != BREAK)
					allOnBreak = false;
			}
		}
		/*all picture clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < picClerkIndex; ++i) {
				if (picClerkLines[i].bribeLineCount
						+ picClerkLines[i].regularLineCount > 0) {
					picClerkLines[i].state = AVAILABLE;

					Signal(picClerkLines[i].breakCV, picLineLock);

					/*printf("%s has woken up a PictureClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up a PictureClerk.\n", 29, ConsoleOutput);
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

				/*printf("%s has woken up an ApplicationClerk\n",
				 managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up an ApplicationClerk.\n", 35,
				ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (appClerkLines[i].state != BREAK)
					allOnBreak = false;
			}
		}
		/*all app clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < appClerkIndex; ++i) {
				if (appClerkLines[i].bribeLineCount
						+ appClerkLines[i].regularLineCount > 0) {
					appClerkLines[i].state = AVAILABLE;

					Signal(appClerkLines[i].breakCV, appLineLock);

					/*printf("%s has woken up an ApplicationClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up an ApplicationClerk.\n", 35,
					ConsoleOutput);
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
							|| senatorInProcess == true)) {
				passportClerkLines[i].state = AVAILABLE;
				Signal(passportClerkLines[i].breakCV, passportLineLock);

				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a PassportClerk.\n", 31, ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (passportClerkLines[i].state != BREAK)
					allOnBreak = false;
			}
		}

		/*all passport clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < passportClerkIndex; ++i) {
				if (passportClerkLines[i].bribeLineCount
						+ passportClerkLines[i].regularLineCount > 0) {
					passportClerkLines[i].state = AVAILABLE;
					Signal(passportClerkLines[i].breakCV, passportLineLock);
					/*printf("%s has woken up an PassportClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up a PassportClerk.\n", 31,
					ConsoleOutput);
				}
			}
		}

		allOnBreak = true;

		Release(passportLineLock);
		Acquire(cashierLineLock);

		for (i = 0; i < cashierIndex; ++i) {

			if (cashierLines[i].state == BREAK
					&& (cashierLines[i].lineCount > 2
							|| senatorInProcess == true)) {
				cashierLines[i].state = AVAILABLE;
				Signal(cashierLines[i].breakCV, cashierLineLock);
				/*printf("%s has woken up a Cashier\n", managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a Cashier.\n", 25, ConsoleOutput);
				allOnBreak = false;
			}
			else {
				if (cashierLines[i].state != BREAK)
					allOnBreak = false;
			}
		}

		/*all cashier clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (j = 0; j < cashierIndex; ++j) {
				if (cashierLines[j].lineCount > 0) {
					cashierLines[j].state = AVAILABLE;
					Signal(cashierLines[j].breakCV, cashierLineLock);
					/*printf("%s has woken up a Cashier\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up a Cashier.\n", 25, ConsoleOutput);
				}
			}
		}

		allOnBreak = true;
		Release(cashierLineLock);

		for (i = 0; i < Rand() % 500; ++i) {
			Yield();
		}
	}

	Exit(0);
}

void picAppCustomerProcess(int customerIndex) {
	int i;
	int myLine = -1;
	int chosePic = -1;
	int lineSize = 1000;
	int bribeChance = Rand() % 5;

	if (customers[customerIndex].picDone == false
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

	if (myLine >= 0) {
		if (chosePic == 1) {

			if (customers[customerIndex].money >= 600) {

				if (bribeChance == 0) {
					picClerkLines[myLine].bribeLineCount++;
					/* printf("%s has gotten in bribe line for %s.\n",
					 customers[customerIndex].name,
					 picClerkLines[myLine].name); */
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the bribe line for PictureClerk ", 47,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);

					customers[customerIndex].money -= 500;
					picClerkLines[myLine].money += 500;
					/* printf("%s has received $500 from %s.\n",
					 picClerkLines[myLine].name, customers[customerIndex].name); */
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
					/* printf("%s has gotten in regular line for %s.\n",
					 customers[customerIndex].name,
					 picClerkLines[myLine].name); */
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
				/* printf("%s has gotten in regular line for %s.\n",
				 customers[customerIndex].name,
				 picClerkLines[myLine].name); */
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
		else {

			if (customers[customerIndex].money >= 600) {

				if (bribeChance == 0) {
					appClerkLines[myLine].bribeLineCount++;
					/* printf("%s has gotten in bribe line for %s.\n",
					 customers[customerIndex].name,
					 appClerkLines[myLine].name); */
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the bribe line for AppClerk ", 43,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);
					customers[customerIndex].money -= 500;
					appClerkLines[myLine].money += 500;
					/* printf("%s has received $500 from %s.\n",
					 appClerkLines[myLine].name, customers[customerIndex].name); */
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
					/* printf("%s has gotten in regular line for %s.\n",
					 customers[customerIndex]->name,
					 appClerkLines[myLine]->name); */
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
				/* printf("%s has gotten in regular line for %s.\n",
				 customers[customerIndex].name,
				 appClerkLines[myLine].name); */
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
		}
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

	if (myLine >= 0) {
		if (customers[customerIndex].money >= 600) {
			if (bribeChance == 0) {
				passportClerkLines[myLine].bribeLineCount++;
				/* printf("%s has gotten in bribe line for %s.\n",
				 customers[customerIndex].name,
				 passportClerkLines[myLine].name); */
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in the bribe line for PassportClerk ", 48,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				customers[customerIndex].money -= 500;
				passportClerkLines[myLine].money += 500;
				/* printf("%s has received $500 from %s.\n",
				 passportClerkLines[myLine].name, customers[customerIndex]->name); */
				Write("PassportClerk ", 15, ConsoleOutput);
				PrintInt(myLine);
				Write(" has received $500 from Customer ", 33, ConsoleOutput);
				PrintInt(customerIndex);
			}
			else {
				passportClerkLines[myLine].regularLineCount++;
				/* printf("%s has gotten in regular line for %s.\n",
				 customers[customerIndex].name,
				 passportClerkLines[myLine].name); */
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in regular line for PassportClerk ", 46,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				Wait(passportClerkLines[myLine].regularLineCV,
						passportLineLock);
				passportClerkLines[myLine].regularLineCount--;
			}
		}
		else {

			/* printf("%s has gotten in regular line for %s.\n",
			 customers[customerIndex].name,
			 passportClerkLines[myLine].name); */
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customerIndex);
			Write(" has gotten in regular line for PassportClerk ", 46,
			ConsoleOutput);
			PrintInt(myLine);
			Write(".\n", 2, ConsoleOutput);
			passportClerkLines[myLine].regularLineCount++;
			Wait(passportClerkLines[myLine].regularLineCV, passportLineLock);
			passportClerkLines[myLine].regularLineCount--;
		}
	}
	passportClerkLines[myLine].state = BUSY;

	Release(passportLineLock);
	passportClerkTransaction(customerIndex, myLine);
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

	if (myLine >= 0) {
		cashierLines[myLine].lineCount++;
		/* printf("%s has gotten in regular line for %s.\n", customers[customerIndex]->name,
		 cashierLines[myLine]->name); */
		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customerIndex);
		Write(" has gotten in regular line for Cashier ", 40, ConsoleOutput);
		PrintInt(myLine);
		Write(".\n", 2, ConsoleOutput);

		Wait(cashierLines[myLine].lineCV, cashierLineLock);
		cashierLines[myLine].lineCount--;

		cashierLines[myLine].state = BUSY;

		Release(cashierLineLock);
		cashierTransaction(customerIndex, myLine);
	}
}

void picClerkTransaction(int customer, int clerk) {
	/* Set the clerk's current customer */
	Acquire(picClerkLines[clerk].transactionLock);
	picClerkLines[clerk].customer = customer;

	/* send SSN */
	/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
	 customers[customer]->SSN, picClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customerIndex);
	Write(" has given SSN ", 25, ConsoleOutput);
	PrintInt(customers[customerIndex].SSN);
	Write(" to PictureClerk ", 17, ConsoleOutput);
	PrintInt(clerk);
	Write(".\n", 2, ConsoleOutput);

	Signal(picClerkLines[clerk].transactionCV,
			picClerkLines[clerk].transactionLock);

	/* take picture

	 Wait to be shown picture
	 Take picture until he likes it*/

	while (customers[customer].picDone == false) {
		Wait(picClerkLines[clerk].transactionCV,
				picClerkLines[clerk].transactionLock);

		if (Rand() % 2 == 0) {
			/*printf("%s does not like their picture from %s.\n",
			 currentThread->getName(), picClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customerIndex);
			Write(" does not like their picture from PictureClerk ", 47,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);
			Signal(picClerkLines[clerk].transactionCV,
					picClerkLines[clerk].transactionLock);
		}
		else {
			/*printf("%s does like their picture from %s.\n",
			 currentThread->getName(), picClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customerIndex);
			Write(" does like their picture from PictureClerk ", 43,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);

			customers[customer].picDone = true;
			Signal(picClerkLines[clerk].transactionCV,
					picClerkLines[clerk].transactionLock);
		}
	}
	Wait(picClerkLines[clerk].transactionCV,
			picClerkLines[clerk].transactionLock);

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
	 picClerkLines[clerk]->name);*/

	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customerIndex);
	Write(" is leaving PictureClerk ", 25, ConsoleOutput);
	PrintInt(clerk);
	Write("'s counter.\n", 12, ConsoleOutput);

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
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" has given SSN ", 25, ConsoleOutput);
	PrintInt(customers[customerIndex].SSN);
	Write(" to AppClerk ", 13, ConsoleOutput);
	PrintInt(clerk);
	Write(".\n", 2, ConsoleOutput);

	Signal(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	/*printf("%s waiting for clerk %s to file the application.\n",
	 currentThread->getName(), appClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is waiting for clerk ", 22, ConsoleOutput);
	PrintInt(clerk);
	Write("to file the application.\n", 25, ConsoleOutput);
	Wait(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	Signal(appClerkLines[clerk].transactionCV,
			appClerkLines[clerk].transactionLock);

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
	 appClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving ApplicationClerk ", 29, ConsoleOutput);
	PrintInt(clerk);
	Write("'s counter.\n", 12, ConsoleOutput);

	Release(appClerkLines[clerk].transactionLock);
}

void passportClerkTransaction(int customer, int clerk) {
	int bribeChance = Rand() % 5;

	while (passportClerkLines[clerk].approved == false) {

		Acquire(passportClerkLines[clerk].transactionLock);
		passportClerkLines[clerk].customer = customer;

		/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
		 customers[customer]->SSN, passportClerkLines[clerk]->name);*/
		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has given SSN ", 25, ConsoleOutput);
		PrintInt(customers[customerIndex].SSN);
		Write(" to PassportClerk ", 18, ConsoleOutput);
		PrintInt(clerk);
		Write(".\n", 2, ConsoleOutput);

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
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customer);
			Write(" is leaving PassportClerk ", 26, ConsoleOutput);
			PrintInt(clerk);
			Write("'s counter.\n", 12, ConsoleOutput);

			Signal(passportClerkLines[clerk].transactionCV,
					passportClerkLines[clerk].transactionLock);

			passportClerkLines[clerk].approved = false;

			Release(passportClerkLines[clerk].transactionLock);

			break;
		}

		/*printf("%s has gone to %s too soon. They are going to the back of the line.\n", currentThread->getName(),
		 passportClerkLines[clerk]->name);*/
		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has gone to PassportClerk ", 27, ConsoleOutput);
		PrintInt(clerk);
		Write(" too soon. They are going to the back of the line.\n", 51,
		ConsoleOutput);

		/* the 5% chance of the passport clerk "making a mistake" happened and we must get back into line*/
		Release(passportClerkLines[clerk].transactionLock);
		Acquire(passportLineLock);

		if (bribeChance == 0) { /*decided to bribe*/
			passportClerkLines[clerk].bribeLineCount++;
			/*printf("%s has gotten in bribe line for %s.\n",
			 customers[customer]->name, passportClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customer);
			Write(" has gotten in the bribe line for PassportClerk ", 48,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);

			customers[customer].money -= 500;
			passportClerkLines[clerk].money += 500;
			/*printf("%s has received $500 from %s.\n",
			 passportClerkLines[clerk]->name, customers[customer]->name);*/
			Write("PassportClerk ", 15, ConsoleOutput);
			PrintInt(clerk);
			Write(" has received $500 from Customer ", 33, ConsoleOutput);
			PrintInt(customer);

			Wait(passportClerkLines[clerk].bribeLineCV, passportLineLock);
			passportClerkLines[clerk].bribeLineCount--;
		}
		else { /*decided not to bribe*/
			passportClerkLines[clerk].regularLineCount++;
			/*printf("%s has gotten in regular line for %s.\n",
			 customers[customer]->name, passportClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customer);
			Write(" has gotten in regular line for PassportClerk ", 46,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);
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

		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has given SSN ", 25, ConsoleOutput);
		PrintInt(customers[customerIndex].SSN);
		Write(" to Cashier ", 12, ConsoleOutput);
		PrintInt(cashier);
		Write(".\n", 2, ConsoleOutput);

		Wait(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);

		/*printf("%s has given %s $100.\n", currentThread->getName(),
		 cashierLines[cashier]->name);*/

		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has given Cashier ", 29, ConsoleOutput);
		PrintInt(cashier);
		Write(" $100.\n", 7, ConsoleOutput);

		Signal(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);
		Wait(cashierLines[cashier].transactionCV,
				cashierLines[cashier].transactionLock);
	}

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
	 cashierLines[cashier]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving Cashier ", 20, ConsoleOutput);
	PrintInt(cashier);
	Write("'s counter.\n", 12, ConsoleOutput);

	Signal(cashierLines[cashier].transactionCV,
			cashierLines[cashier].transactionLock);
	Release(cashierLines[cashier].transactionLock);
}

int main() {
	int sen;
	int i;
	int option;
	char c;
	bool validinput;

	NUM_CUSTOMERS = 5;
	NUM_SENATORS = 0;
	NUM_PIC_CLERKS = 1;
	NUM_APP_CLERKS = 1;
	NUM_PP_CLERKS = 1;
	NUM_CASHIERS = 1;
	NUM_MANAGERS = 1;
	sen = NUM_SENATORS;

	picLineLock = CreateLock("Pic Line Lock", 13);
	appLineLock = CreateLock("App Line Lock", 13);
	passportLineLock = CreateLock("Passport Line Lock", 18);
	cashierLineLock = CreateLock("Cashier Line Lock", 17);
	customerCounterLock = CreateLock("Customer Counter Lock", 21);
	managerCounterLock = CreateLock("Manager Counter Lock", 20);
	cashierCounterLock = CreateLock("Cashier Counter Lock", 20);

	senatorLock = CreateLock("Senator Lock", 12);
	/* Semaphore senatorSema("Senator Semaphore", 1); */
	senatorCV = CreateCondition("Senator CV", 10);

	picClerkIndexLock = CreateLock("picClerkIndexLock", 17);
	appClerkIndexLock = CreateLock("appClerkIndexLock", 17);
	passportClerkIndexLock = CreateLock("ppClerkIndexLock", 16);
	customerIndexLock = CreateLock("customerIndexLock", 17);
	cashierIndexLock = CreateLock("cashierIndexLock", 16);

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		Fork(bePassportClerk);
	}

	for (i = 0; i < NUM_CASHIERS; ++i) {
		Fork(beCashier);
	}

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
