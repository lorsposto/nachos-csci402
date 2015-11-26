#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, ppClerkIndexLock, ppLineLock, regularLineCV, bribeLineCV, transactionCV, transactionLock, breakCV,
	bribeMonitorIndex, regularMonitorIndex, ppMonitorIndex, ppCustomerIndex, customer, money;

clerkState state;

int getBribeLineCount() {
	return GetMonitor(bribeMonitorIndex, myIndex);
}

int getRegularLineCount() {
	return GetMonitor(regularMonitorIndex, myIndex);
}

int getCurrentCustomer() {
	return GetMonitor(ppCustomerIndex, myIndex);
}

int main() {

	bool approved = false;
	state = AVAILABLE;

	ppClerkIndexLock = CreateLock("PassClerkIndexLock", 18);
	ppLineLock = CreateLock("PassClerkLineLock", 14);
	regularLineCV = CreateCondition("PassClerkRegularCV", 18);
	bribeLineCV = CreateCondition("PassClerkBribeCV", 16);
	transactionCV = CreateCondition("PassClerkTransactionCV", 22);
	transactionLock = CreateLock("PassClerkTransactionLock", 24);
	breakCV = CreateCondition("PassClerkBreakCV", 16);
	bribeMonitorIndex = CreateMonitor("PassBribeLines", 14, 1);
	regularMonitorIndex = CreateMonitor("PassRegularLines", 16, 1);
	ppMonitorIndex = CreateMonitor("PassClerkCount", 14, 100);
	ppCustomerIndex = CreateMonitor("PassCustomerIndex", 17, 100);

	Acquire(ppClerkIndexLock);
	myIndex = GetMonitor(ppMonitorIndex, 0);
	Release(ppClerkIndexLock);

	customer = -1;
	money = 0;

	while(1) {
		while (state != BREAK) {
			Acquire(ppLineLock);
			if(getBribeLineCount() > 0) {
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(bribeLineCV, ppLineLock);
				state == BUSY;
			} else if (getRegularLineCount() > 0) {
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(regularLineCV, ppLineLock);
				state = BUSY;
			} else {
				state = BREAK;
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(breakCV, appLineLock);
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(appLineLock);
				break;
			}

			Acquire(transactionLock);
			Release(appLineLock);

			Wait(transactionCV, transactionLock);

			Write("ApplicationClerk ", 17, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" .\n", 3, ConsoleOutput);

			for (i = 0; i < 80; ++i) {
				Yield();
			}

			Write("ApplicationClerk ", 17, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has recorded a complete application for Customer ", 50,
			ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(".\n", 2, ConsoleOutput);

			/* set application as complete */
			SetMonitor(customerApprovedList, getCurrentCustomer(), 1);
			Signal(transactionCV, transactionLock);
			Wait(transactionCV, transactionLock);
			Release(transactionLock);
		}
	}

	Exit(0);
}
