#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	CUSTOMER, SENATOR
} customerType;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int NUM_CUSTOMERS = 0;
int NUM_SENATORS = 0;
int NUM_PIC_CLERKS = 0;
int NUM_APP_CLERKS = 0;
int NUM_PP_CLERKS = 0;
int NUM_CASHIERS = 0;
int NUM_MANAGERS = 0;

int myIndex, i, appClerkIndexLock, appLineLock, customerCounterLock, customerCounter, customerIndex,
	appClerkStates,	appRegularLineCVs, appBribeLineCVs, appTransactionCVs, appTransactionLocks,
	appBreakCVs, appBribeMonitorIndex, appRegularMonitorIndex, appMonitorIndex,
	appCustomerIndex, customer, money, picClerkIndexLock, picClerkStates,
	picLineLock, picRegularLineCVs, picBribeLineCVs, picTransactionCVs,
	picTransactionLocks, picBreakCVs, picBribeMonitorIndex, picRegularMonitorIndex,
	picMonitorIndex, picCustomerIndex, passportClerkIndexLock, passportLineLock, passportClerkStates,
	passportRegularLineCVs, passportBribeLineCVs, passportTransactionCVs, passportTransactionLocks, passportBreakCVs,
	passportBribeMonitorIndex, passportRegularMonitorIndex, passportMonitorIndex, passportCustomerIndex,
	customers, customerMoney, appClerkMoney, picClerkMoney, passportClerkMoney, cashierMoney,
	customerAppDoneList, customerPicDoneList, customerPassportDoneList, customerCashierDoneList, cashierIndexLock, cashierBreakCVs,
	cashierMonitorIndex, cashierRegularMonitorIndex, cashierLineLock, cashierRegularLineCVs, cashierTransactionCVs,
	cashierTransactionLocks, cashierStates, cashierCustomerIndex, customerSSNs, customerEarlyList, customerTypes,
	senatorInProcess, currentSenator, currentSenatorSSN, senatorCV, senatorLock;

int getAppBribeLineCount() {
	return GetMonitor(GetMonitor(appBribeMonitorIndex, myIndex), myIndex);
}

void setAppBribeLineCount(int count) {
	SetMonitor(GetMonitor(appBribeMonitorIndex, myIndex), myIndex, count);
}

int getAppRegularLineCount() {
	return GetMonitor(GetMonitor(appRegularMonitorIndex, myIndex), myIndex);
}

void setAppRegularLineCount(int count) {
	SetMonitor(GetMonitor(appRegularMonitorIndex, myIndex), myIndex, count);
}

int getCurrentAppCustomer() {
	return GetMonitor(appCustomerIndex, myIndex);
}

int getPicBribeLineCount() {
	return GetMonitor(GetMonitor(picBribeMonitorIndex, myIndex), myIndex);
}

void setPicBribeLineCount(int count) {
	SetMonitor(GetMonitor(picBribeMonitorIndex, myIndex), myIndex, count);
}

int getPicRegularLineCount() {
	return GetMonitor(GetMonitor(picRegularMonitorIndex, myIndex), myIndex);
}

void setPicRegularLineCount(int count) {
	SetMonitor(GetMonitor(picRegularMonitorIndex, myIndex), myIndex, count);
}

int getCurrentPicCustomer() {
	return GetMonitor(picCustomerIndex, myIndex);
}

int getPassportBribeLineCount() {
	return GetMonitor(GetMonitor(passportBribeMonitorIndex, myIndex), myIndex);
}

void setPassportBribeLineCount(int count) {
	SetMonitor(GetMonitor(passportBribeMonitorIndex, myIndex), myIndex, count);
}

int getPassportRegularLineCount() {
	return GetMonitor(GetMonitor(passportRegularMonitorIndex, myIndex), myIndex);
}

void setPassportRegularLineCount(int count) {
	SetMonitor(GetMonitor(passportRegularMonitorIndex, myIndex), myIndex, count);
}

int getCurrentPassportCustomer() {
	return GetMonitor(passportCustomerIndex, myIndex);
}

int getCustomerMoney() {
	return GetMonitor(customerMoney, myIndex);
}

int getPicClerkMoney() {
	return GetMonitor(picClerkMoney, myIndex);
}

int getAppClerkMoney() {
	return GetMonitor(appClerkMoney, myIndex);
}

int getPassportClerkMoney() {
	return GetMonitor(passportClerkMoney, myIndex);
}

int getCashierMoney() {
	return GetMonitor(cashierMoney, myIndex);
}

int getCashierLineCount() {
	return GetMonitor(GetMonitor(cashierMonitorIndex, myIndex), myIndex);
}

void setCashierLineCount(int count) {
	SetMonitor(GetMonitor(cashierMonitorIndex, myIndex), myIndex, count);
}

void picClerkTransaction(int customer, int clerk) {
	/* Set the clerk's current customer */
	Acquire(GetMonitor(picTransactionLocks, clerk));
	SetMonitor(picCustomerIndex, clerk, customer);

	/* send SSN */
	/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
	 customers[customer]->SSN, picClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customerIndex);
	Write(" has given SSN ", 25, ConsoleOutput);
	PrintInt(GetMonitor(customerSSNs, customer));
	Write(" to PictureClerk ", 17, ConsoleOutput);
	PrintInt(clerk);
	Write(".\n", 2, ConsoleOutput);

	Signal(GetMonitor(picTransactionCVs, clerk),
			GetMonitor(picTransactionLocks, clerk));

	/* take picture

	 Wait to be shown picture
	 Take picture until he likes it*/

	while (GetMonitor(customerPicDoneList, customer) == false) {
		Wait(GetMonitor(picTransactionCVs, clerk),
				GetMonitor(picTransactionLocks, clerk));

		if (Rand() % 2 == 0) {
			/*printf("%s does not like their picture from %s.\n",
			 currentThread->getName(), picClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customerIndex);
			Write(" does not like their picture from PictureClerk ", 47,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);
			Signal(GetMonitor(picTransactionCVs, clerk),
					GetMonitor(picTransactionLocks, clerk));
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

			SetMonitor(customerPicDoneList, customer, true);
			Signal(GetMonitor(picTransactionCVs, clerk),
					GetMonitor(picTransactionLocks, clerk));
		}
	}
	Wait(GetMonitor(picTransactionCVs, clerk),
			GetMonitor(picTransactionLocks, clerk));

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
	 picClerkLines[clerk]->name);*/

	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customerIndex);
	Write(" is leaving PictureClerk ", 25, ConsoleOutput);
	PrintInt(clerk);
	Write("'s counter.\n", 12, ConsoleOutput);

	Signal(GetMonitor(picTransactionCVs, clerk),
			GetMonitor(picTransactionLocks, clerk));
	Release(GetMonitor(picTransactionLocks, clerk));
}

void appClerkTransaction(int customer, int clerk) {
	/* Set the clerk's current customer */

	Acquire(GetMonitor(appTransactionLocks, clerk));
	SetMonitor(appCustomerIndex, clerk, customer);
	/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
	 customers[customer]->SSN, appClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" has given SSN ", 25, ConsoleOutput);
	PrintInt(GetMonitor(customerSSNs, customer));
	Write(" to AppClerk ", 13, ConsoleOutput);
	PrintInt(clerk);
	Write(".\n", 2, ConsoleOutput);

	Signal(GetMonitor(appTransactionCVs, clerk),
			GetMonitor(appTransactionLocks, clerk));

	/*printf("%s waiting for clerk %s to file the application.\n",
	 currentThread->getName(), appClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is waiting for clerk ", 22, ConsoleOutput);
	PrintInt(clerk);
	Write("to file the application.\n", 25, ConsoleOutput);
	Wait(GetMonitor(appTransactionCVs, clerk),
			GetMonitor(appTransactionLocks, clerk));

	Signal(GetMonitor(appTransactionCVs, clerk),
			GetMonitor(appTransactionLocks, clerk));

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
	 appClerkLines[clerk]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving ApplicationClerk ", 29, ConsoleOutput);
	PrintInt(clerk);
	Write("'s counter.\n", 12, ConsoleOutput);

	Release(GetMonitor(appTransactionLocks, clerk));
}

void passportClerkTransaction(int customer, int clerk) {
	int bribeChance = Rand() % 5;

	while (GetMonitor(customerPassportDoneList, customer) == false) {

		Acquire(GetMonitor(passportTransactionLocks, clerk));
		SetMonitor(passportCustomerIndex, clerk, customer);

		/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
		 customers[customer]->SSN, passportClerkLines[clerk]->name);*/
		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has given SSN ", 25, ConsoleOutput);
		PrintInt(GetMonitor(customerSSNs, customer));
		Write(" to PassportClerk ", 18, ConsoleOutput);
		PrintInt(clerk);
		Write(".\n", 2, ConsoleOutput);

		Signal(GetMonitor(passportTransactionCVs, clerk),
				GetMonitor(passportTransactionLocks, clerk));

		Wait(GetMonitor(passportTransactionCVs, clerk),
				GetMonitor(passportTransactionLocks, clerk));

		if (GetMonitor(customerPassportDoneList, customer) == true) {
			/*stop here so we don't hop back in line*/

			/*giving info*/
			Signal(GetMonitor(passportTransactionCVs, clerk),
				GetMonitor(passportTransactionLocks, clerk));

			/*waiting to see if application is approved*/
			Wait(GetMonitor(passportTransactionCVs, clerk),
				GetMonitor(passportTransactionLocks, clerk));

			/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
			 passportClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customer);
			Write(" is leaving PassportClerk ", 26, ConsoleOutput);
			PrintInt(clerk);
			Write("'s counter.\n", 12, ConsoleOutput);

			Signal(GetMonitor(passportTransactionCVs, clerk),
				GetMonitor(passportTransactionLocks, clerk));

			SetMonitor(customerPassportDoneList, customer, false);

			Release(GetMonitor(passportTransactionLocks, clerk));

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
		Release(GetMonitor(passportTransactionLocks, clerk));
		Acquire(passportLineLock);

		if (bribeChance == 0) { /*decided to bribe*/
			setPassportBribeLineCount(getPassportBribeLineCount() + 1);
			/*printf("%s has gotten in bribe line for %s.\n",
			 customers[customer]->name, passportClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customer);
			Write(" has gotten in the bribe line for PassportClerk ", 48,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);

			SetMonitor(customerMoney, customer, getCustomerMoney() - 500);
			SetMonitor(passportClerkMoney, clerk, getPassportClerkMoney() + 500);
			/*printf("%s has received $500 from %s.\n",
			 passportClerkLines[clerk]->name, customers[customer]->name);*/
			Write("PassportClerk ", 15, ConsoleOutput);
			PrintInt(clerk);
			Write(" has received $500 from Customer ", 33, ConsoleOutput);
			PrintInt(customer);

			Wait(GetMonitor(passportBribeLineCVs, clerk), passportLineLock);
			setPassportBribeLineCount(getPassportBribeLineCount() - 1);
		}
		else { /*decided not to bribe*/
			setPassportRegularLineCount(getPassportRegularLineCount() + 1);
			/*printf("%s has gotten in regular line for %s.\n",
			 customers[customer]->name, passportClerkLines[clerk]->name);*/
			Write("Customer ", 9, ConsoleOutput);
			PrintInt(customer);
			Write(" has gotten in regular line for PassportClerk ", 46,
			ConsoleOutput);
			PrintInt(clerk);
			Write(".\n", 2, ConsoleOutput);
			Wait(GetMonitor(passportRegularLineCVs, clerk), passportLineLock);
			setPassportRegularLineCount(getPassportRegularLineCount() - 1);
		}

		Release(passportLineLock);
	}
}

void cashierTransaction(int customer, int cashier) {
	/* Set the cashier's current customer */
	Acquire(GetMonitor(cashierTransactionLocks, cashier));
	SetMonitor(cashierCustomerIndex, cashier, customer);

	Signal(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));

	Wait(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));

	if (GetMonitor(customerCashierDoneList, customer) == true) {
		Signal(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));

		/*printf("%s has given SSN %i to %s\n", customers[customer]->name,
		 customers[customer]->SSN, cashierLines[cashier]->name);*/

		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has given SSN ", 25, ConsoleOutput);
		PrintInt(GetMonitor(customerSSNs, customer));
		Write(" to Cashier ", 12, ConsoleOutput);
		PrintInt(cashier);
		Write(".\n", 2, ConsoleOutput);

		Wait(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));

		/*printf("%s has given %s $100.\n", currentThread->getName(),
		 cashierLines[cashier]->name);*/

		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customer);
		Write(" has given Cashier ", 29, ConsoleOutput);
		PrintInt(cashier);
		Write(" $100.\n", 7, ConsoleOutput);

		Signal(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));
		Wait(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));
	}

	/*printf("%s is leaving %s's counter.\n", currentThread->getName(),
	 cashierLines[cashier]->name);*/
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving Cashier ", 20, ConsoleOutput);
	PrintInt(cashier);
	Write("'s counter.\n", 12, ConsoleOutput);

	Signal(GetMonitor(cashierTransactionCVs, cashier),
			GetMonitor(cashierTransactionLocks, cashier));
	Release(GetMonitor(cashierTransactionLocks, cashier));
}

void picAppCustomerProcess(int customerIndex) {
	int i;
	int myLine = -1;
	int chosePic = -1;
	int lineSize = 1000;
	int bribeChance = Rand() % 5;

	if (GetMonitor(customerPicDoneList, customerIndex) == false
			&& GetMonitor(customerAppDoneList, customerIndex) == false) {

		Acquire(picLineLock);
		Acquire(appLineLock);

		if (Rand() % 2 == 0) {
			Release(appLineLock);

			chosePic = 1;

			for (i = 0; i < NUM_PIC_CLERKS; ++i) {
				if (getPicRegularLineCount()
						+ getPicBribeLineCount() < lineSize) {
					myLine = i;
					lineSize = getPicRegularLineCount()
						+ getPicBribeLineCount();
				}
			}
		}
		else {
			Release(picLineLock);

			chosePic = 0;

			for (i = 0; i < NUM_APP_CLERKS; ++i) {
				if (getPicRegularLineCount()
						+ getPicBribeLineCount() < lineSize) {
					myLine = i;
					lineSize = getPicRegularLineCount()
						+ getPicBribeLineCount();
				}
			}
		}
	}
	else if (GetMonitor(customerPicDoneList, customerIndex) == false) {
		Acquire(picLineLock);
		chosePic = 1;

		for (i = 0; i < NUM_PIC_CLERKS; ++i) {
			if (getPicRegularLineCount()
						+ getPicBribeLineCount() < lineSize) {
				myLine = i;
				lineSize = getPicRegularLineCount()
						+ getPicBribeLineCount();
			}
		}
	}
	else if (GetMonitor(customerAppDoneList, customerIndex) == false) {
		Acquire(appLineLock);

		chosePic = 0;

		for (i = 0; i < NUM_APP_CLERKS; ++i) {
			if (getAppRegularLineCount()
						+ getAppBribeLineCount() < lineSize) {
				myLine = i;
				lineSize = getAppRegularLineCount()
						+ getAppBribeLineCount();
			}
		}
	}

	if (myLine >= 0) {
		if (chosePic == 1) {

			if (GetMonitor(customerMoney, customerIndex) >= 600) {

				if (bribeChance == 0) {
					SetMonitor(picBribeMonitorIndex, myLine,
						getPicBribeLineCount() + 1);
					/* printf("%s has gotten in bribe line for %s.\n",
					 customers[customerIndex].name,
					 picClerkLines[myLine].name); */
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the bribe line for PictureClerk ", 47,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);

					SetMonitor(customerMoney, customerIndex, getCustomerMoney() - 500);
					SetMonitor(picClerkMoney, myLine, getPicClerkMoney() + 500);
					/* printf("%s has received $500 from %s.\n",
					 picClerkLines[myLine].name, customers[customerIndex].name); */
					Write("PictureClerk ", 13, ConsoleOutput);
					PrintInt(myLine);
					Write(" has received $500 from Customer ", 33,
					ConsoleOutput);
					PrintInt(customerIndex);
					Write(".\n", 2, ConsoleOutput);
					Wait(GetMonitor(picBribeLineCVs, myLine), picLineLock);
					setPicBribeLineCount(getPicBribeLineCount() - 1);
				}
				else {
					setPicRegularLineCount(getPicRegularLineCount() + 1);
					/* printf("%s has gotten in regular line for %s.\n",
					 customers[customerIndex].name,
					 picClerkLines[myLine].name); */
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the regular line for PictureClerk ",
							49, ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);

					Wait(GetMonitor(picRegularLineCVs, myLine), picLineLock);
					setPicRegularLineCount(getPicRegularLineCount() - 1);
				}

			}
			else {
				setPicRegularLineCount(getPicRegularLineCount() + 1);
				/* printf("%s has gotten in regular line for %s.\n",
				 customers[customerIndex].name,
				 picClerkLines[myLine].name); */
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in the regular line for PictureClerk ", 49,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				Wait(GetMonitor(picRegularLineCVs, myLine), picLineLock);
				setPicRegularLineCount(getPicRegularLineCount() - 1);
			}

			SetMonitor(picClerkStates, myLine, BUSY);

			Release(picLineLock);
			picClerkTransaction(customerIndex, myLine);
		}
		else {

			if (getCustomerMoney() >= 600) {

				if (bribeChance == 0) {
					setAppBribeLineCount(getAppBribeLineCount() + 1);
					/* printf("%s has gotten in bribe line for %s.\n",
					 customers[customerIndex].name,
					 appClerkLines[myLine].name); */
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in the bribe line for AppClerk ", 43,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);
					SetMonitor(customerMoney, customerIndex, getCustomerMoney() - 500);
					SetMonitor(appClerkMoney, myLine, getAppClerkMoney() + 500);
					/* printf("%s has received $500 from %s.\n",
					 appClerkLines[myLine].name, customers[customerIndex].name); */
					Write("AppClerk ", 9, ConsoleOutput);
					PrintInt(myLine);
					Write(" has received $500 from Customer ", 33,
					ConsoleOutput);
					PrintInt(customerIndex);

					Wait(GetMonitor(appBribeLineCVs, myLine), appLineLock);
					setAppBribeLineCount(getAppBribeLineCount() - 1);
				}
				else {
					setAppRegularLineCount(getAppRegularLineCount() + 1);
					/* printf("%s has gotten in regular line for %s.\n",
					 customers[customerIndex]->name,
					 appClerkLines[myLine]->name); */
					Write("Customer ", 9, ConsoleOutput);
					PrintInt(customerIndex);
					Write(" has gotten in regular line for AppClerk ", 41,
					ConsoleOutput);
					PrintInt(myLine);
					Write(".\n", 2, ConsoleOutput);
					Wait(GetMonitor(appRegularLineCVs, myLine), appLineLock);
					setAppRegularLineCount(getAppRegularLineCount() - 1);
				}
			}
			else {
				setAppRegularLineCount(getAppRegularLineCount() + 1);
				/* printf("%s has gotten in regular line for %s.\n",
				 customers[customerIndex].name,
				 appClerkLines[myLine].name); */
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in regular line for AppClerk ", 41,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				Wait(GetMonitor(appRegularLineCVs, myLine), appLineLock);
				setAppRegularLineCount(getAppRegularLineCount() - 1);
			}

			SetMonitor(appClerkStates, myLine, BUSY);

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
		if (getPassportRegularLineCount() < lineSize) {
			myLine = i;
			lineSize = getPassportRegularLineCount();
		}
	}

	if (myLine >= 0) {
		if (getCustomerMoney() >= 600) {
			if (bribeChance == 0) {
				setPassportBribeLineCount(getPassportBribeLineCount() + 1);
				/* printf("%s has gotten in bribe line for %s.\n",
				 customers[customerIndex].name,
				 passportClerkLines[myLine].name); */
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in the bribe line for PassportClerk ", 48,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				SetMonitor(customerMoney, customerIndex, getCustomerMoney() - 500);
				SetMonitor(passportClerkMoney, myLine, getPassportClerkMoney() + 500);
				/* printf("%s has received $500 from %s.\n",
				 passportClerkLines[myLine].name, customers[customerIndex]->name); */
				Write("PassportClerk ", 15, ConsoleOutput);
				PrintInt(myLine);
				Write(" has received $500 from Customer ", 33, ConsoleOutput);
				PrintInt(customerIndex);
			}
			else {
				setPassportRegularLineCount(getPassportRegularLineCount() + 1);
				/* printf("%s has gotten in regular line for %s.\n",
				 customers[customerIndex].name,
				 passportClerkLines[myLine].name); */
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(customerIndex);
				Write(" has gotten in regular line for PassportClerk ", 46,
				ConsoleOutput);
				PrintInt(myLine);
				Write(".\n", 2, ConsoleOutput);

				Wait(GetMonitor(passportRegularLineCVs, myLine),
						passportLineLock);
				setPassportRegularLineCount(getPassportRegularLineCount() - 1);
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
			setPassportRegularLineCount(getPassportRegularLineCount() + 1);
			Wait(GetMonitor(passportRegularLineCVs, myLine), passportLineLock);
			setPassportRegularLineCount(getPassportRegularLineCount() - 1);
		}
	}
	SetMonitor(passportClerkStates, myLine, BUSY);

	Release(passportLineLock);
	passportClerkTransaction(customerIndex, myLine);
}

void cashierCustomerProcess(int customerIndex) {
	int myLine = -1;
	int lineSize = 1000;
	int i;

	Acquire(cashierLineLock);

	for (i = 0; i < NUM_CASHIERS; ++i) {
		if (getCashierLineCount() < lineSize) {
			myLine = i;
			lineSize = getCashierLineCount();
		}
	}

	if (myLine >= 0) {
		setCashierLineCount(getCashierLineCount() + 1);
		/* printf("%s has gotten in regular line for %s.\n", customers[customerIndex]->name,
		 cashierLines[myLine]->name); */
		Write("Customer ", 9, ConsoleOutput);
		PrintInt(customerIndex);
		Write(" has gotten in regular line for Cashier ", 40, ConsoleOutput);
		PrintInt(myLine);
		Write(".\n", 2, ConsoleOutput);

		Wait(GetMonitor(cashierRegularLineCVs, myLine), cashierLineLock);
		setCashierLineCount(getCashierLineCount() - 1);

		SetMonitor(cashierStates, myLine, BUSY);

		Release(cashierLineLock);
		cashierTransaction(customerIndex, myLine);
	}
}

int main() {

	customerIndex = CreateMonitor("CustomerIndex", 13, 1);
	customerCounter = CreateMonitor("CustomerCounter", 15, 1);

	customers = CreateMonitor("Customers", 9, 100);
	customerSSNs = CreateMonitor("CustomerSSNs", 12, 100);
	customerEarlyList = CreateMonitor("CustomerEarlyList", 17, 100);
	customerTypes = CreateMonitor("CustomerTypes", 13, 100);

	customerMoney = CreateMonitor("CustomerMoney", 13, 100);
	appClerkMoney = CreateMonitor("AppClerkMoney", 13, 100);
	picClerkMoney = CreateMonitor("PicClerkMoney", 13, 100);
	passportClerkMoney = CreateMonitor("PassportClerkMoney", 18, 100);
	cashierMoney = CreateMonitor("CashierMoney", 12, 100);

	picClerkStates = CreateMonitor("PicClerkStates", 14, 100);
	appClerkStates = CreateMonitor("AppClerkStates", 14, 100);
	passportClerkStates = CreateMonitor("PassportClerkStates", 19, 100);
	cashierStates = CreateMonitor("CashierStates", 13, 100);

	appClerkIndexLock = CreateLock("AppClerkIndexLock", 17);
	appLineLock = CreateLock("AppClerkLineLock", 16);
	appMonitorIndex = CreateMonitor("AppClerkCount", 13, 100);
	appCustomerIndex = CreateMonitor("AppCustomerIndex", 16, 100);
	appBribeMonitorIndex = CreateMonitor("AppClerkBribeLineNum", 20, 100);
	appRegularMonitorIndex = CreateMonitor("AppClerkRegularLineNum", 22, 100);
	customerAppDoneList = CreateMonitor("CustomerAppDoneList", 19, 100);
	appBribeLineCVs = CreateMonitor("AppClerkBribeCV", 15, 100);
	appRegularLineCVs = CreateMonitor("AppClerkRegularCV", 17, 100);
	appTransactionCVs = CreateMonitor("AppClerkTransactionCV", 21, 100);
	appTransactionLocks = CreateMonitor("AppClerkTransactionLock", 23, 100);
	appBreakCVs = CreateMonitor("AppClerkBreakCV", 15, 100);

	picClerkIndexLock = CreateLock("PicClerkIndexLock", 17);
	picLineLock = CreateLock("PicClerkLineLock", 16);
	picRegularLineCVs = CreateMonitor("PicClerkRegularCV", 17, 100);
	picBribeLineCVs = CreateMonitor("PicClerkBribeCV", 15, 100);
	picTransactionCVs = CreateMonitor("PicClerkTransactionCV", 21, 100);
	picTransactionLocks = CreateMonitor("PicClerkTransactionLock", 23, 100);
	picBreakCVs = CreateMonitor("PicClerkBreakCV", 15, 100);
	picBribeMonitorIndex = CreateMonitor("PicBribeLineNum", 15, 100);
	picRegularMonitorIndex = CreateMonitor("PicRegularLineNum", 17, 100);
	picMonitorIndex = CreateMonitor("PicClerkCount", 13, 100);
	picCustomerIndex = CreateMonitor("PicCustomerIndex", 16, 100);
	customerPicDoneList = CreateMonitor("CustomerPicDoneList", 19, 100);

	passportClerkIndexLock = CreateLock("PassClerkIndexLock", 18);
	passportLineLock = CreateLock("PassClerkLineLock", 17);
	passportRegularLineCVs = CreateMonitor("PassClerkRegularCV", 18, 100);
	passportBribeLineCVs = CreateMonitor("PassClerkBribeCV", 16, 100);
	passportTransactionCVs = CreateMonitor("PassClerkTransactionCV", 22, 100);
	passportTransactionLocks = CreateMonitor("PassClerkTransactionLock", 24, 100);
	passportBreakCVs = CreateMonitor("PassClerkBreakCV", 16, 100);
	passportBribeMonitorIndex = CreateMonitor("PassBribeLineNum", 14, 1);
	passportRegularMonitorIndex = CreateMonitor("PassRegularLineNum", 16, 1);
	passportMonitorIndex = CreateMonitor("PassClerkCount", 14, 100);
	passportCustomerIndex = CreateMonitor("PassCustomerIndex", 17, 100);
	customerPassportDoneList = CreateMonitor("CustomerPassportDoneList", 24, 100);

	cashierIndexLock = CreateLock("CashierIndexLock", 16);
	cashierLineLock = CreateLock("CashierLineLock", 15);
	cashierRegularLineCVs = CreateMonitor("CashierRegularCV", 16, 100);
	cashierTransactionCVs = CreateMonitor("CashierTransactionCV", 20, 100);
	cashierTransactionLocks = CreateMonitor("CashierTransactionLock", 22, 100);
	cashierBreakCVs = CreateMonitor("CashierBreakCV", 14, 100);
	cashierRegularMonitorIndex = CreateMonitor("CashierRegularLineNum", 21, 100);
	cashierMonitorIndex = CreateMonitor("CashierClerkCount", 17, 100);
	cashierCustomerIndex = CreateMonitor("CashierCustomerIndex", 20, 100);
	customerCashierDoneList = CreateMonitor("CustomerCashierDoneList", 23, 100);

	senatorLock = CreateLock("SenatorLock", 11);
	senatorCV = CreateCondition("SenatorCV", 9);
	currentSenatorSSN = CreateMonitor("CurrentSenatorSSN", 17, 1);
	currentSenator = CreateMonitor("CurrentSenator", 14, 1);
	senatorInProcess = CreateMonitor("SenatorInProcess", 16, 1);

	SetMonitor(customerSSNs, customer, customerIndex);
	SetMonitor(customerPicDoneList, customer, false);
	SetMonitor(customerAppDoneList, customer, false);
	SetMonitor(customerPassportDoneList, customer, false);
	SetMonitor(customerCashierDoneList, customer, false);
	SetMonitor(customerEarlyList, customer, false);
	SetMonitor(customerMoney, customer, 100);
	SetMonitor(customerTypes, customer, CUSTOMER);

	SetMonitor(customerIndex, 0, GetMonitor(customerIndex, 0) + 1);

	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is entering the passport office.\n", 34, ConsoleOutput);
	if (GetMonitor(customerTypes, customer) == SENATOR) {
		/* senatorSema.P(); */
		Acquire(GetMonitor(senatorLock, 0));
		SetMonitor(senatorInProcess, 0, true);
		SetMonitor(currentSenator, 0, GetMonitor(customers, customer));
		Release(GetMonitor(senatorLock, 0));
	}

	while (GetMonitor(customerPicDoneList, customer) == false
			|| GetMonitor(customerAppDoneList, customer) == false
			|| GetMonitor(customerPassportDoneList, customer) == false
			|| GetMonitor(customerCashierDoneList, customer) == false) {

		if (GetMonitor(customerTypes, customer) != SENATOR && GetMonitor(senatorInProcess, 0) == true) {
			Acquire(GetMonitor(senatorLock, 0));
			/* Write("%s is going outside the Passport Office because there is a Senator present.\n",
			 currentThread->getName()); */
			Wait(GetMonitor(senatorCV, 0), GetMonitor(senatorLock, 0));
			Release(GetMonitor(senatorLock, 0));
		}
		if (GetMonitor(senatorInProcess, 0) == false
				|| (GetMonitor(customerTypes, customer) == SENATOR
						&& GetMonitor(currentSenatorSSN, 0) == GetMonitor(customerSSNs, customer))) {

			if (GetMonitor(customerPicDoneList, customer) == false
					|| GetMonitor(customerAppDoneList, customer) == false) {
				picAppCustomerProcess(customer);
			}
			else if (GetMonitor(customerAppDoneList, customer) == true
					&& GetMonitor(customerPicDoneList, customer) == true
					&& GetMonitor(customerPassportDoneList, customer) == false
					&& GetMonitor(customerEarlyList, customer) == false) {
				passportCustomerProcess(customer);

			}
			else if (GetMonitor(customerPassportDoneList, customer) == true
					|| (GetMonitor(customerEarlyList, customer) == true
							&& GetMonitor(customerCashierDoneList, customer) == false)) {
				cashierCustomerProcess(customer);
			}

			if (GetMonitor(customerTypes, customer) != SENATOR
					&& GetMonitor(senatorInProcess, 0) == true) {
				Acquire(GetMonitor(senatorLock, 0));
				Wait(GetMonitor(senatorCV, 0), GetMonitor(senatorLock, 0));
				Release(GetMonitor(senatorLock, 0));
			}
		}
	}

	/* Write("%s is leaving the Passport Office.\n",
	 customers[customerIndex]->name); */
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving the Passport Office\n", 32, ConsoleOutput);

	Acquire(customerCounterLock);
	SetMonitor(customerCounter, 0, GetMonitor(customerCounter, 0) + 1);
	Release(customerCounterLock);

	if (GetMonitor(customerTypes, customer) == SENATOR && GetMonitor(senatorInProcess, 0) == true) {
		Acquire(GetMonitor(senatorLock, 0));
		/* currentSenator = NULL; */
		SetMonitor(senatorInProcess, 0, false);
		Broadcast(GetMonitor(senatorCV, 0), GetMonitor(senatorLock, 0));
		Release(GetMonitor(senatorLock, 0));
		/* senatorSema.V(); */
	}

	Exit(0);

}
