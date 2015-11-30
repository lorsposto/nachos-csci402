#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, appClerkIndexLock, appLineLock, regularLineCVs, bribeLineCVs, transactionCVs, transactionLocks, breakCVs,
	bribeMonitorIndex, regularMonitorIndex, appMonitorIndex, appCustomerIndex, customerAppDoneList, customer, money;

clerkState state;

int getBribeLineCount() {
	return GetMonitor(GetMonitor(bribeMonitorIndex, myIndex), myIndex);
}

int getRegularLineCount() {
	return GetMonitor(GetMonitor(regularMonitorIndex, myIndex), myIndex);
}

int getCurrentCustomer() {
	return GetMonitor(appCustomerIndex, myIndex);
}

int main() {

	bool approved = false;
	state = AVAILABLE;

	appClerkIndexLock = CreateLock("AppClerkIndexLock", 17);
	appLineLock = CreateLock("AppClerkLineLock", 16);
	appMonitorIndex = CreateMonitor("AppClerkCount", 13, 100);
	appCustomerIndex = CreateMonitor("AppCustomerIndex", 16, 100);
	bribeMonitorIndex = CreateMonitor("AppClerkBribeLineNum", 20, 100);
	regularMonitorIndex = CreateMonitor("AppClerkRegularLineNum", 22, 100);
	customerAppDoneList = CreateMonitor("CustomerAppDoneList", 19, 100);
	bribeLineCVs = CreateMonitor("AppClerkBribeCV", 15, 100);
	regularLineCVs = CreateMonitor("AppClerkRegularCV", 17, 100);
	transactionCVs = CreateMonitor("AppClerkTransactionCV", 21, 100);
	transactionLocks = CreateMonitor("AppClerkTransactionLock", 23, 100);
	breakCVs = CreateMonitor("AppClerkBreakCV", 15, 100);

	Acquire(appClerkIndexLock);
	myIndex = GetMonitor(appMonitorIndex, 0);
	Release(appClerkIndexLock);

	customer = -1;
	money = 0;

	while(1) {
		while (state != BREAK) {
			Acquire(appLineLock);
			if(getBribeLineCount() > 0) {
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(bribeLineCVs, myIndex), appLineLock);
				state == BUSY;
			} else if (getRegularLineCount() > 0) {
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(regularLineCVs, myIndex), appLineLock);
				state = BUSY;
			} else {
				state = BREAK;
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(GetMonitor(breakCVs, myIndex), appLineLock);
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(appLineLock);
				break;
			}

			Acquire(GetMonitor(transactionLocks, myIndex));
			Release(appLineLock);

			Wait(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));

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
			SetMonitor(customerAppDoneList, getCurrentCustomer(), 1);
			Signal(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));
			Wait(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));
			Release(GetMonitor(transactionLocks, myIndex));
		}
	}

	Exit(0);
}
