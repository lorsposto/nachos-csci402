#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, picClerkIndexLock, picLineLock, regularLineCVs, bribeLineCVs, transactionCVs, transactionLocks, breakCVs,
	bribeMonitorIndex, regularMonitorIndex, picMonitorIndex, picCustomerIndex, customerApprovedList, 
	customerPicDoneList, customer, money;

clerkState state;

int getBribeLineCount() {
	return GetMonitor(bribeMonitorIndex, myIndex);
}

int getRegularLineCount() {
	return GetMonitor(regularMonitorIndex, myIndex);
}

int getCurrentCustomer() {
	return GetMonitor(picCustomerIndex, myIndex);
}

int main() {

	bool firstTime = true;
	state = AVAILABLE;

	picClerkIndexLock = CreateLock("PicClerkIndexLock", 17);
	picLineLock = CreateLock("PicClerkLineLock", 13);
	regularLineCVs = CreateMonitor("PicClerkRegularCV", 17, 100);
	bribeLineCVs = CreateMonitor("PicClerkBribeCV", 15, 100);
	transactionCVs = CreateMonitor("PicClerkTransactionCV", 21, 100);
	transactionLocks = CreateMonitor("PicClerkTransactionLock", 23, 100);
	breakCVs = CreateMonitor("PicClerkBreakCV", 15, 100);
	bribeMonitorIndex = CreateMonitor("PicBribeLineNum", 15, 100);
	regularMonitorIndex = CreateMonitor("PicRegularLineNum", 17, 100);
	picMonitorIndex = CreateMonitor("PicClerkCount", 13, 100);
	picCustomerIndex = CreateMonitor("PicCustomerIndex", 16, 100);
	customerPicDoneList = CreateMonitor("CustomerPicDoneList", 20, 100);

	Acquire(picClerkIndexLock);
	myIndex = GetMonitor(picMonitorIndex, 0);
	Release(picClerkIndexLock);

	customer = -1;
	money = 0;

	while(1) {
		while (state != BREAK) {
			Acquire(picLineLock);
			if(getBribeLineCount() > 0) {
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(bribeLineCVs, myIndex), picLineLock);
				state == BUSY;
			} else if (getRegularLineCount() > 0) {
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(regularLineCVs, myIndex), picLineLock);
				state = BUSY;
			} else {
				state = BREAK;
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(GetMonitor(breakCVs, myIndex), picLineLock);
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(picLineLock);
				break;
			}

			Acquire(GetMonitor(transactionLocks, myIndex));
			Release(picLineLock);

			Wait(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));

			Write("PictureClerk ", 17, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" .\n", 3, ConsoleOutput);

			/* Doing job, customer waiting, signal when done */
			while (GetMonitor(customerPicDoneList, getCurrentCustomer()) == 0) {
				if (!firstTime) {
					Write("PictureClerk ", 13, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has been told that Customer ", 29, ConsoleOutput);
					PrintInt(getCurrentCustomer());
					Write(" does not like their picture.\n", 30, ConsoleOutput);
				}

				Write("PictureClerk ", 13, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has taken a picture of Customer ", 33, ConsoleOutput);
				PrintInt(getCurrentCustomer());
				Write(".\n", 2, ConsoleOutput);

				Signal(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));
				/* Waiting for customer to accept photo */
				Wait(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));

				firstTime = false;
			}

			Write("PictureClerk ", 13, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has been told that Customer ", 29, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" does like their picture.\n", 26, ConsoleOutput);

			Signal(GetMonitor(transactionCVs, myIndex),
				GetMonitor(transactionLocks, myIndex));

			Wait(GetMonitor(transactionCVs, myIndex),
				GetMonitor(transactionLocks, myIndex));

			Release(GetMonitor(transactionLocks, myIndex));
		}
	}

	Exit(0);
}
