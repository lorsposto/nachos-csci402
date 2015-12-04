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

char myBribeCV[22] = " PicClerkBribeCV";
char myRegularCV[22] = " PicClerkRegCV";
char myTransactionCV[22] = " PicClerkTransCV";
char myRegularCount[22] = " PicClerkRegLineCount";
char myBribeCount[22] = " PicClerkBribeLineCount";
char myTransactionLock[22] = " PicClerkTransLock"; 

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
	regularLineCVs = CreateMonitor("PicClerkRegCV", 13, 100);
	bribeLineCVs = CreateMonitor("PicClerkBribeCV", 15, 100);
	transactionCVs = CreateMonitor("PicClerkTransCV", 15, 100);
	transactionLocks = CreateMonitor("PicClerkTransLock", 17, 100);
	breakCVs = CreateMonitor("PicClerkBreakCV", 15, 100);
	bribeMonitorIndex = CreateMonitor("PicBribeLineNum", 15, 100);
	regularMonitorIndex = CreateMonitor("PicRegLineNum", 13, 100);
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

	SetMonitor(bribeLineCVs, myIndex, CreateCondition(myBribeCV, 22));
	SetMonitor(regularLineCVs, myIndex, CreateCondition(myRegularCV, 22));
	SetMonitor(transactionCVs, myIndex, CreateCondition(myTransactionCV, 22));
	SetMonitor(regularMonitorIndex, myIndex, 0);
	SetMonitor(bribeMonitorIndex, myIndex, 0);
	SetMonitor(transactionLocks, myIndex, CreateLock(myTransactionLock, 22));
	SetMonitor(stateIndex, myIndex, 0); /*set state to available */

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
