#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, picClerkIndexLock, picLineLock, regularLineCVs, bribeLineCVs, transactionCVs, transactionLocks, breakCVs,
	bribeMonitorIndex, regularMonitorIndex, picMonitorIndex, picCustomerIndex, customerPicDoneList, customer, money, stateIndex;

clerkState state;

char myBribeCV[32] = " PicClerkBribeCV";
char myRegularCV[32] = " PicClerkRegularCV";
char myTransactionCV[32] = " PicClerkTransactionCV";
char myRegularCount[32] = " PicClerkRegularLineCount";
char myBribeCount[32] = " PicClerkBribeLineCount";
char myTransactionLock[32] = " PicClerkTransactionLock";


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

	picClerkIndexLock = CreateLock("PicClerkIndexLock", 17);
	picLineLock = CreateLock("PicClerkLineLock", 16);
	regularLineCVs = CreateMonitor("PicClerkRegularCV", 17, 100);
	bribeLineCVs = CreateMonitor("PicClerkBribeCV", 15, 100);
	transactionCVs = CreateMonitor("PicClerkTransactionCV", 21, 100);
	transactionLocks = CreateMonitor("PicClerkTransactionLock", 23, 100);
	breakCVs = CreateMonitor("PicClerkBreakCV", 15, 100);
	bribeMonitorIndex = CreateMonitor("PicBribeLineNum", 15, 100);
	regularMonitorIndex = CreateMonitor("PicRegularLineNum", 17, 100);
	stateIndex = CreateMonitor("PicState", 8, 100);
	picMonitorIndex = CreateMonitor("PicClerkCount", 13, 100);
	picCustomerIndex = CreateMonitor("PicCustomerIndex", 16, 100);
	customerPicDoneList = CreateMonitor("CustomerPicDoneList", 19, 100);

	Acquire(picClerkIndexLock);
	myIndex = GetMonitor(picMonitorIndex, 0);
	Release(picClerkIndexLock);

	myBribeCV[0] = myIndex + '0';
	myRegularCV[0] = myIndex + '0';
	myTransactionCV[0] = myIndex + '0';
	myRegularCount[0] = myIndex + '0';
	myBribeCount[0] = myIndex + '0';
	myTransactionLock[0] = myIndex + '0';

	SetMonitor(bribeLineCVs, myIndex, CreateMonitor(myBribeCV, 32, 1));
	SetMonitor(regularLineCVs, myIndex, CreateMonitor(myRegularCV, 32, 1));
	SetMonitor(transactionCVs, myIndex, CreateMonitor(myTransactionCV, 32, 1));
	SetMonitor(regularMonitorIndex, myIndex, CreateMonitor(myRegularCount, 32, 1));
	SetMonitor(bribeMonitorIndex, myIndex, CreateMonitor(myBribeCount, 32, 1));
	SetMonitor(transactionLocks, myIndex, CreateMonitor(myTransactionLock, 32, 1));
	SetMonitor(stateIndex, myIndex, 0); /*set state to available*/

	while(1) {
		while (GetMonitor(stateIndex, myIndex) != 2) {
			Acquire(picLineLock);
			if(getBribeLineCount() > 0) {
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(bribeLineCVs, myIndex), picLineLock);
				SetMonitor(stateIndex, myIndex, 1);
			} else if (getRegularLineCount() > 0) {
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(regularLineCVs, myIndex), picLineLock);
				SetMonitor(stateIndex, myIndex, 1);
			} /*else {
				SetMonitor(stateIndex, myIndex, 2);
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(GetMonitor(breakCVs, myIndex), picLineLock);
				Write("PictureClerk ", 17, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(picLineLock);
				break;
			}*/

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
