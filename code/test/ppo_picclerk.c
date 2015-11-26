#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, picClerkIndexLock, picLineLock, regularLineCV, bribeLineCV, transactionCV, transactionLock, breakCV,
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
	regularLineCV = CreateCondition("PicClerkRegularCV", 17);
	bribeLineCV = CreateCondition("PicClerkBribeCV", 15);
	transactionCV = CreateCondition("PicClerkTransactionCV", 21);
	transactionLock = CreateLock("PicClerkTransactionLock", 23);
	breakCV = CreateCondition("PicClerkBreakCV", 15);
	bribeMonitorIndex = CreateMonitor("PicBribeLines", 13, 1);
	regularMonitorIndex = CreateMonitor("PicRegularLines", 15, 1);
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
				Signal(bribeLineCV, picLineLock);
				state == BUSY;
			} else if (getRegularLineCount() > 0) {
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(regularLineCV, picLineLock);
				state = BUSY;
			} else {
				state = BREAK;
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(breakCV, picLineLock);
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(picLineLock);
				break;
			}

			Acquire(transactionLock);
			Release(picLineLock);

			Wait(transactionCV, transactionLock);

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

				Signal(transactionCV,
						transactionLock);
				/* Waiting for customer to accept photo */
				Wait(transactionCV,
						transactionLock);

				firstTime = false;
			}

			Write("PictureClerk ", 13, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has been told that Customer ", 29, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" does like their picture.\n", 26, ConsoleOutput);

			Signal(transactionCV,
					transactionLock);

			Wait(transactionCV,
					transactionLock);
			Release(transactionLock);
		}
	}

	Exit(0);
}
