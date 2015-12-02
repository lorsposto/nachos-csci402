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

char myBribeCV[32] = " PassClerkBribeCV";
char myRegularCV[32] = " PassClerkRegularCV";
char myTransactionCV[32] = " PassClerkTransactionCV";
char myRegularCount[32] = " PassClerkRegularLineCount";
char myBribeCount[32] = " PassClerkBribeLineCount";
char myTransactionLock[32] = " PassClerkTransactionLock";

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

	passClerkIndexLock = CreateLock("PassClerkIndexLock", 18);
	ppLineLock = CreateLock("PassClerkLineLock", 17);
	regularLineCVs = CreateMonitor("PassClerkRegularCV", 18, 100);
	bribeLineCVs = CreateMonitor("PassClerkBribeCV", 18, 100);
	transactionCVs = CreateMonitor("PassClerkTransactionCV", 22, 100);
	transactionLocks = CreateMonitor("PassClerkTransactionLock", 24, 100);
	breakCVs = CreateMonitor("PassClerkBreakCV", 16, 100);
	bribeMonitorIndex = CreateMonitor("PassBribeLineNum", 16, 100);
	regularMonitorIndex = CreateMonitor("PassRegularLineNum", 18, 100);
	passMonitorIndex = CreateMonitor("PassClerkCount", 14, 1);
	passCustomerIndex = CreateMonitor("PassCustomerIndex", 17, 100);
	customerApprovedList = CreateMonitor("CustomerApprovedList", 20, 100);
	passStateIndex = CreateMonitor("PassState", 9, 100);

	Acquire(passClerkIndexLock);
	myIndex = GetMonitor(passMonitorIndex, 0);
	Release(passClerkIndexLock);

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
			} else {
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
			}

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
