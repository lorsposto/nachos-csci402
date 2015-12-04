#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, passClerkIndexLock, ppLineLock, regularLineCVs, bribeLineCVs, transactionCVs, transactionLocks, breakCVs,
	bribeMonitorIndex, regularMonitorIndex, passMonitorIndex, passCustomerIndex, customer, money, customerApprovedList,
	passStateIndex;

char myBribeCV[22] = " PpClerkBribeCV";
char myRegularCV[22] = " PpClerkRegCV";
char myTransactionCV[22] = " PpClerkTransCV";
char myRegularCount[22] = " PpClerkRegLineCount";
char myBribeCount[22] = " PpClerkBribeLineCount";
char myTransactionLock[22] = " PpClerkTransLock";

clerkState state;

int getBribeLineCount() {
	return GetMonitor(bribeMonitorIndex, myIndex);
}

int getRegularLineCount() {
	return GetMonitor(regularMonitorIndex, myIndex);
}

int getCurrentCustomer() {
	return GetMonitor(passCustomerIndex, myIndex);
}

int main() {

	bool approved = false;

	passClerkIndexLock = CreateLock("PpClerkIndexLock", 16);
	ppLineLock = CreateLock("PpClerkLineLock", 15);
	regularLineCVs = CreateMonitor("PpClerkRegCV", 12, 100);
	bribeLineCVs = CreateMonitor("PpClerkBribeCV", 14, 100);
	transactionCVs = CreateMonitor("PpClerkTransCV", 14, 100);
	transactionLocks = CreateMonitor("PpClerkTransLock", 16, 100);
	breakCVs = CreateMonitor("PpClerkBreakCV", 14, 100);
	bribeMonitorIndex = CreateMonitor("PpBribeLineNum", 14, 100);
	regularMonitorIndex = CreateMonitor("PpRegLineNum", 12, 100);
	passMonitorIndex = CreateMonitor("PpClerkCount", 12, 1);
	passCustomerIndex = CreateMonitor("PpCustomerIndex", 15, 100);
	customerApprovedList = CreateMonitor("CustPpDoneList", 14, 100);
	passStateIndex = CreateMonitor("PpState", 7, 100);

	Acquire(passClerkIndexLock);
	myIndex = GetMonitor(passMonitorIndex, 0);
	Release(passClerkIndexLock);

	myBribeCV[0] = myIndex + '0';
	myRegularCV[0] = myIndex + '0';
	myTransactionCV[0] = myIndex + '0';
	myRegularCount[0] = myIndex + '0';
	myBribeCount[0] = myIndex + '0';
	myTransactionLock[0] = myIndex + '0';

	SetMonitor(bribeLineCVs, myIndex, CreateMonitor(myBribeCV, 22, 1));
	SetMonitor(regularLineCVs, myIndex, CreateMonitor(myRegularCV, 22, 1));
	SetMonitor(transactionCVs, myIndex, CreateMonitor(myTransactionCV, 22, 1));
	SetMonitor(regularMonitorIndex, myIndex, CreateMonitor(myRegularCount, 22, 1));
	SetMonitor(bribeMonitorIndex, myIndex, CreateMonitor(myBribeCount, 22, 1));
	SetMonitor(transactionLocks, myIndex, CreateMonitor(myTransactionLock, 22, 1));
	SetMonitor(passStateIndex, myIndex, 0);

	while(1) {
		while (GetMonitor(passStateIndex, myIndex) != 2) {
			Acquire(ppLineLock);
			if(getBribeLineCount() > 0) {
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(bribeLineCVs, myIndex), ppLineLock);
				SetMonitor(passStateIndex, myIndex, 1);
			} else if (getRegularLineCount() > 0) {
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);
				Signal(GetMonitor(regularLineCVs, myIndex), ppLineLock);
				SetMonitor(passStateIndex, myIndex, 1);
			} /*else {
				SetMonitor(passStateIndex, myIndex, 2);
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);
				Wait(GetMonitor(breakCVs, myIndex), ppLineLock);
				Write("PassportClerk ", 14, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);
				Release(ppLineLock);
				break;
			}*/

			Acquire(GetMonitor(transactionLocks, myIndex));
			Release(ppLineLock);

			Wait(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));

			Write("PassportClerk ", 14, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" .\n", 3, ConsoleOutput);

			for (i = 0; i < 80; ++i) {
				Yield();
			}

			Write("PassportClerk ", 17, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has recorded a complete application for Customer ", 50,
			ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(".\n", 2, ConsoleOutput);

			/* set application as complete */
			SetMonitor(customerApprovedList, getCurrentCustomer(), 1);
			Signal(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));
			Wait(GetMonitor(transactionCVs, myIndex), GetMonitor(transactionLocks, myIndex));
			Release(GetMonitor(transactionLocks, myIndex));
		}
	}

	Exit(0);
}
