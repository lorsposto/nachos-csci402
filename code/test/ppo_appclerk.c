#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, appClerkIndexLock, appLineLock, regularLineCV, bribeLineCV, transactionCV, transactionLock, breakCV,
	bribeMonitorIndex, regularMonitorIndex, appMonitorIndex, appCustomerIndex, customerApprovedList, customer, money;

clerkState state;

int getBribeLineCount() {
	return GetMonitor(bribeMonitorIndex, myIndex);
}

int getRegularLineCount() {
	return GetMonitor(regularMonitorIndex, myIndex);
}

int getCurrentCustomer() {
	return GetMonitor(appCustomerIndex, myIndex);
}

int main() {

	bool approved = false;
	state = AVAILABLE;

	appClerkIndexLock = CreateLock("AppClerkIndexLock", 17);
	appLineLock = CreateLock("AppLineLock", 13);
	regularLineCV = CreateCondition("AppClerkRegularCV", 17);
	bribeLineCV = CreateCondition("AppClerkBribeCV", 15);
	transactionCV = CreateCondition("AppClerkTransactionCV", 21);
	transactionLock = CreateLock("AppClerkTransactionLock", 23);
	breakCV = CreateCondition("AppClerkBreakCV", 15);
	bribeMonitorIndex = CreateMonitor("AppBribeLines", 13, 1);
	regularMonitorIndex = CreateMonitor("AppRegularLines", 15, 1);
	appMonitorIndex = CreateMonitor("AppClerkCount", 13, 100);
	appCustomerIndex = CreateMonitor("AppCustomerIndex", 16, 100);
	customerApprovedList = CreateMonitor("CustomerApprovedList", 20, 100);

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
				Signal(bribeLineCV, appLineLock);
				state == BUSY;
			} else if (getRegularLineCount() > 0) {
				Write("ApplicationClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(regularLineCV, appLineLock);
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
