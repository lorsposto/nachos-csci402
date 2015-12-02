#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int myIndex, i, cashierIndexLock, cashierLineLock, lineCVs, transactionCVs, transactionLocks, breakCVs,
 	lineMonitorIndex, cashierMonitorIndex, cashierCustomerIndex, customerEarlybirdList, customerApprovedList, customerPicDoneList, 
 	customerAppDoneList, customerPassDoneList, cashierMoneys, customerMoneys, customerGotPassport, customerCertified, cashierApproved,
 	cashStateIndex, customer, money;

clerkState state;

char myBribeCV[32] = " CashierBribeCV";
char myRegularCV[32] = " CashierRegularCV";
char myTransactionCV[32] = " CashierTransactionCV";
char myLineCount[32] = " CashierLineCount";
char myTransactionLock[32] = " CashierTransactionLock";

int getCurrentCustomer() {
	return GetMonitor(cashierCustomerIndex, myIndex);
}

int main() {
	cashierIndexLock = CreateLock("CashierIndexLock", 16);
	cashierLineLock = CreateLock("CashierLineLock", 15);
	lineCVs = CreateMonitor("CashierLineCV", 13, 100);
	transactionCVs = CreateMonitor("CashierTransactionCV", 20, 100);
	transactionLocks = CreateMonitor("CashierTransactionLock", 21, 100);
	breakCVs = CreateMonitor("CashierBreakCV", 14, 100);
	lineMonitorIndex = CreateMonitor("CashierLineNum", 14, 100);
	cashierMonitorIndex = CreateMonitor("CashierClerkCount", 17, 100);
	cashierCustomerIndex = CreateMonitor("CashierCustomerIndex", 19, 100);
	customerPicDoneList = CreateMonitor("CustomerPicDoneList", 19, 100); /* ==picdone */
	customerAppDoneList = CreateMonitor("CustomerAppDoneList", 19, 100); /* == appdone */
	customerPassDoneList = CreateMonitor("CustomerPassportDoneList", 20, 100);
	customerEarlybirdList = CreateMonitor("CustomerEarlybirdList", 21, 100); /* == earlybird */
	customerApprovedList = CreateMonitor("CustomerCashierDoneList", 20, 100); 
	cashierMoneys = CreateMonitor("CashierMoneyList", 16, 100); 
	customerMoneys = CreateMonitor("CustomerMoneyList", 17, 100); 
	customerGotPassport = CreateMonitor("CustomerGotPassportList", 23, 100); 
	customerCertified = CreateMonitor("CustomerCertifiedList", 21, 100); 
	cashierApproved = CreateMonitor("CashierApprovedList", 20, 100); 
	cashStateIndex = CreateMonitor("CashierState", 12, 100);

	cashierIndexLock = CreateLock("CashierIndexLock", 16);
	cashierMonitorIndex = CreateMonitor("CashierClerkCount", 17, 100);

	Acquire(cashierIndexLock);
	myIndex = GetMonitor(cashierMonitorIndex, 0);
	Release(cashierIndexLock);

	myBribeCV[0] = myIndex + '0';
	myRegularCV[0] = myIndex + '0';
	myTransactionCV[0] = myIndex + '0';
	myLineCount[0] = myIndex + '0';
	myTransactionLock[0] = myIndex + '0';

	SetMonitor(lineCVs, myIndex, CreateMonitor(myLineCount, 32, 1));
	SetMonitor(transactionCVs, myIndex, CreateMonitor(myTransactionCV, 32, 1));
	SetMonitor(lineMonitorIndex, myIndex, CreateMonitor(myLineCount, 32, 1));
	SetMonitor(transactionLocks, myIndex, CreateMonitor(myTransactionLock, 32, 1));
	SetMonitor(cashStateIndex, myIndex, 0);

	while (1) {
		/* once they are not on break, process the line*/
		while (GetMonitor(cashStateIndex, myIndex) != 2) {
			Acquire(cashierLineLock);

			if (GetMonitor(lineMonitorIndex, myIndex) > 0) {

				/*printf(
				 "%s has signalled a Customer to come to their counter.\n",
				 cashierLines[cashierIndex]->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has signalled a Customer to come to their counter.\n",
						52, ConsoleOutput);

				Signal(GetMonitor(lineCVs, myIndex), cashierLineLock);
				SetMonitor(cashStateIndex, myIndex, 1);
			}/*else {
				state = BREAK;
				SetMonitor(cashStateIndex, myIndex, 2);
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is going on break.\n", 20, ConsoleOutput);

				Wait(GetMonitor(breakCVs, myIndex), cashierLineLock);
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" is coming off break.\n", 22, ConsoleOutput);

				Release(cashierLineLock);
				break;
			}*/

			Acquire(GetMonitor(transactionLocks, myIndex));
			Release(cashierLineLock);

			/* wait for Customer data*/
			Wait(GetMonitor(transactionCVs, myIndex),
					GetMonitor(transactionLocks, myIndex));

			Write("Cashier ", 8, ConsoleOutput);
			PrintInt(myIndex);
			Write(" has received SSN ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" from Customer ", 18, ConsoleOutput);
			PrintInt(getCurrentCustomer());
			Write(" .\n", 3, ConsoleOutput);

			/* If the customer has finished everything*/
			if (GetMonitor(customerAppDoneList, myIndex) == 1 &&
					GetMonitor(customerPicDoneList, myIndex) == 1 &&
					GetMonitor(customerPassDoneList, myIndex) == 1) {

				/*printf("%s has verified that %s has been certified by a PassportClerk\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has verified that Customer ", 18, ConsoleOutput);
				PrintInt(GetMonitor(cashierCustomerIndex, myIndex));
				Write(" has been certified by a PassportClerk.\n", 40,
				ConsoleOutput);

				SetMonitor(customerApprovedList, myIndex, 1);

				Signal(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));

				Wait(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));

				Signal(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));
				
				Wait(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));

				/*printf("%s has received the $100 from %s after certification\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has received the $100 from Customer ", 37,
				ConsoleOutput);
				PrintInt(GetMonitor(cashierCustomerIndex, myIndex));
				Write(" after certification.\n", 21, ConsoleOutput);

				/* receive money from customer */
				SetMonitor(cashierMoneys, myIndex, GetMonitor(cashierMoneys, myIndex) + 100);
				SetMonitor(customerMoneys, getCurrentCustomer(), GetMonitor(customerMoneys, getCurrentCustomer()) - 100);

				/* set passport as received by customer */
				SetMonitor(customerGotPassport, getCurrentCustomer(), 1);

				/*printf("%s has provided %s their completed passport\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has provided Customer ", 23, ConsoleOutput);
				PrintInt(getCurrentCustomer());
				Write(" their completed passport.\n", 26, ConsoleOutput);

				Signal(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));

				/*printf(
				 "%s has recorded that %s has been given their completed passport\n",
				 cashierLines[cashierIndex]->name,
				 cashierLines[cashierIndex]->customer->name);*/

				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has recorded that Customer ", 28, ConsoleOutput);
				PrintInt(getCurrentCustomer());
				Write(" has been given their completed passport.\n", 42,
				ConsoleOutput);
			}
			else if (GetMonitor(customerAppDoneList, myIndex) == 1 &&
					GetMonitor(customerPicDoneList, myIndex) == 1 &&
					GetMonitor(customerEarlybirdList, myIndex) == 1) {
				/*printf("%s has received the $100 from %s before certification. They are to go to the back of line\n",
				 cashierLines[cashierIndex]->name, cashierLines[cashierIndex]->customer->name);*/
				Write("Cashier ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has received the $100 from Customer ", 37,
				ConsoleOutput);
				PrintInt(getCurrentCustomer());
				Write(" before certification.\n", 22, ConsoleOutput);

				SetMonitor(customerCertified, getCurrentCustomer(), 1);
				SetMonitor(cashierApproved, myIndex, 0);
				/* artificially allow customer to go next time */
				/*printf("%s has gone to %s too soon. They are going to the back of the line\n",
				 cashierLines[cashierIndex]->customer->name, cashierLines[cashierIndex]->name);*/
				Write("Customer ", 9, ConsoleOutput);
				PrintInt(getCurrentCustomer());
				Write(" has gone to Cashier ", 20, ConsoleOutput);
				PrintInt(myIndex);
				Write(" too soon. They are going to the back of the line.\n",
						51, ConsoleOutput);

				Signal(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));
			}
			else {
				SetMonitor(cashierApproved, myIndex, 0);
				Signal(GetMonitor(transactionCVs, myIndex),
						GetMonitor(transactionLocks, myIndex));
			}
			Wait(GetMonitor(transactionCVs, myIndex),
					GetMonitor(transactionLocks, myIndex));
			Release(GetMonitor(transactionLocks, myIndex));
		}
	}
	Exit(0);
}
