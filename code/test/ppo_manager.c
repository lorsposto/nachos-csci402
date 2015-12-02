#include "syscall.h"

#define NULL 0
typedef enum {
	false, true
} bool;

int i;
int j;
int myIndex, managerIndexLock, managerMonitorIndex, customersServed, numCustomers;
int picLineLock, picMonitorIndex, picBribeMonitorIndex, picRegularMonitorIndex, picStateIndex, picBreakCVs;
int appLineLock, appMonitorIndex, appBribeMonitorIndex, appRegularMonitorIndex, appStateIndex, appBreakCVs;
int passLineLock, passMonitorIndex, passBribeMonitorIndex, passRegularMonitorIndex, passStateIndex, passBreakCVs;
int cashLineLock, cashMonitorIndex, cashLineMonitorIndex, cashStateIndex, cashBreakCVs;
bool allOnBreak;

int main() {

	managerIndexLock = CreateLock("ManagerIndexLock", 16);
	managerMonitorIndex = CreateMonitor("ManagerCount", 12, 1);
	customersServed = CreateMonitor("CustomersServedCount", 20, 1);
	numCustomers = CreateMonitor("CustomerIndex", 13, 1);

	picLineLock = CreateLock("PicClerkLineLock", 16);
	picMonitorIndex = CreateMonitor("PicClerkCount", 13, 100);
	picBribeMonitorIndex = CreateMonitor("PicBribeLineNum", 15, 100);
	picRegularMonitorIndex = CreateMonitor("PicRegularLineNum", 17, 100);
	picStateIndex = CreateMonitor("PicState", 8, 100);
	picBreakCVs = CreateMonitor("PicClerkBreakCV", 15, 100);

	appLineLock = CreateLock("AppClerkLineLock", 16);
	appMonitorIndex = CreateMonitor("AppClerkCount", 13, 100);
	appBribeMonitorIndex = CreateMonitor("AppClerkBribeLineNum", 20, 100);
	appRegularMonitorIndex = CreateMonitor("AppClerkRegularLineNum", 22, 100);
	appStateIndex = CreateMonitor("AppState", 8, 100);
	appBreakCVs = CreateMonitor("AppClerkBreakCV", 15, 100);

	passLineLock = CreateLock("PassClerkLineLock", 17);
	passMonitorIndex = CreateMonitor("PassClerkCount", 14, 100);
	passBribeMonitorIndex = CreateMonitor("PassBribeLineNum", 16, 100);
	passRegularMonitorIndex = CreateMonitor("PassRegularLineNum", 18, 100);
	passStateIndex = CreateMonitor("PassState", 9, 100);
 	passBreakCVs = CreateMonitor("PassClerkBreakCV", 16, 100);

 	cashLineLock = CreateLock("CashierLineLock", 15);
	cashMonitorIndex = CreateMonitor("CashierClerkCount", 17, 100);
	cashLineMonitorIndex = CreateMonitor("CashierLineNum", 14, 100);
	cashStateIndex = CreateMonitor("CashierState", 12, 100);
 	cashBreakCVs = CreateMonitor("CashierBreakCV", 14, 100);

	Acquire(managerIndexLock);
	myIndex = GetMonitor(managerMonitorIndex, 0);
	Release(managerIndexLock);

	while (GetMonitor(customersServed, 0) < GetMonitor(numCustomers, 0)) {

		/*if (managers[myIndex].counter % 2 == 0) {
			broadcastMoney();
		}*/

		/*checks if all clerks are on break. Because the manager terminates when all customers are served,
		 if there are still customers to be served and all clerks of a type are on break we know there are
		 <3 people in line who must be served*/
		allOnBreak = true;
		
		/*managers[myIndex].counter++;*/

		Acquire(picLineLock);
		for (i = 0; i < GetMonitor(picMonitorIndex, 0); ++i) {

			if (GetMonitor(picStateIndex, i) == 2
					&& GetMonitor(picRegularMonitorIndex, i) +
					GetMonitor(picBribeMonitorIndex, i) > 2) {
				SetMonitor(picStateIndex, i, 0);

				Signal(GetMonitor(picBreakCVs, i), picLineLock);

				/*printf("%s has woken up a PictureClerk\n",
				 managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a PictureClerk.\n", 29, ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (GetMonitor(picStateIndex, i) != 2)
					allOnBreak = false;
			}
		}
		/*all picture clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < GetMonitor(picMonitorIndex, 0); ++i) {
				if (GetMonitor(picRegularMonitorIndex, i) +
					GetMonitor(picBribeMonitorIndex, i) > 0) {
					SetMonitor(picStateIndex, i, 0);


					Signal(GetMonitor(picBreakCVs, i), picLineLock);


					/*printf("%s has woken up a PictureClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up a PictureClerk.\n", 29, ConsoleOutput);
				}
			}
		}

		allOnBreak = true;

		Release(picLineLock);

		Acquire(appLineLock);

		for (i = 0; i < GetMonitor(appMonitorIndex, 0); ++i) {

			if (GetMonitor(appStateIndex, i) == 2
					&& GetMonitor(appRegularMonitorIndex, i) +
					GetMonitor(appBribeMonitorIndex, i) > 2) {
				SetMonitor(appStateIndex, i, 0);

				Signal(GetMonitor(appBreakCVs, i), appLineLock);

				/*printf("%s has woken up a PictureClerk\n",
				 managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a PictureClerk.\n", 29, ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (GetMonitor(appStateIndex, i) != 2)
					allOnBreak = false;
			}
		}
		/*all picture clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < GetMonitor(appMonitorIndex, 0); ++i) {
				if (GetMonitor(appRegularMonitorIndex, i) +
					GetMonitor(appBribeMonitorIndex, i) > 0) {
					SetMonitor(appStateIndex, i, 0);


					Signal(GetMonitor(appBreakCVs, i), appLineLock);


					/*printf("%s has woken up a PictureClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up an ApplicationClerk.\n", 35, ConsoleOutput);
				}
			}
		}

		allOnBreak = true;

		Release(appLineLock);

		Acquire(passLineLock);

		for (i = 0; i < GetMonitor(passMonitorIndex, 0); ++i) {

			if (GetMonitor(passStateIndex, i) == 2
					&& GetMonitor(passRegularMonitorIndex, i) +
					GetMonitor(passBribeMonitorIndex, i) > 2) {
				SetMonitor(passStateIndex, i, 0);

				Signal(GetMonitor(passBreakCVs, i), passLineLock);

				/*printf("%s has woken up a PictureClerk\n",
				 managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a PictureClerk.\n", 29, ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (GetMonitor(passStateIndex, i) != 2)
					allOnBreak = false;
			}
		}
		/*all picture clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < GetMonitor(passMonitorIndex, 0); ++i) {
				if (GetMonitor(passRegularMonitorIndex, i) +
					GetMonitor(passBribeMonitorIndex, i) > 0) {
					SetMonitor(passStateIndex, i, 0);


					Signal(GetMonitor(passBreakCVs, i), passLineLock);


					/*printf("%s has woken up a PictureClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up a PassportClerk.\n", 31, ConsoleOutput);
				}
			}
		}

		allOnBreak = true;

		Release(passLineLock);
		Acquire(cashLineLock);

		for (i = 0; i < GetMonitor(cashMonitorIndex, 0); ++i) {

			if (GetMonitor(cashStateIndex, i) == 2
					&& GetMonitor(cashLineMonitorIndex, i) > 2) {
				SetMonitor(cashStateIndex, i, 0);

				Signal(GetMonitor(cashBreakCVs, i), cashLineLock);

				/*printf("%s has woken up a PictureClerk\n",
				 managers[index]->name);*/
				Write("Manager ", 8, ConsoleOutput);
				PrintInt(myIndex);
				Write(" has woken up a Cashier.\n", 25, ConsoleOutput);

				allOnBreak = false;
			}
			else {
				if (GetMonitor(cashStateIndex, i) != 2)
					allOnBreak = false;
			}
		}
		/*all picture clerks are on break because they have < 3 in line, but there are still customers waiting to be served*/
		if (allOnBreak == true) {
			for (i = 0; i < GetMonitor(cashMonitorIndex, 0); ++i) {
				if (GetMonitor(cashLineMonitorIndex, i) > 0) {
					SetMonitor(cashStateIndex, i, 0);


					Signal(GetMonitor(cashBreakCVs, i), cashLineLock);


					/*printf("%s has woken up a PictureClerk\n",
					 managers[index]->name);*/
					Write("Manager ", 8, ConsoleOutput);
					PrintInt(myIndex);
					Write(" has woken up a Cashier.\n", 25, ConsoleOutput);
				}
			}
		}

		allOnBreak = true;
		Release(cashLineLock);

		for (i = 0; i < Rand() % 500; ++i) {
			Yield();
		}
	}

	Exit(0);
}
