#include "syscall.h"

/**
 * Formerly beCustomer
 */
int main() {
	int customer;

	Acquire(customerIndexLock);
	customer = Customer();
	Release(customerIndexLock);
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is entering the passport office.\n", 34, ConsoleOutput);
	if (customers[customer].type == SENATOR) {
		/* senatorSema.P(); */
		Acquire(senatorLock);
		senatorInProcess = true;
		currentSenator = customers[customer];
		Release(senatorLock);
	}

	while ((customers[customer].picDone == false
			|| customers[customer].appDone == false
			|| customers[customer].certified == false
			|| customers[customer].gotPassport == false)) {

		if (customers[customer].type != SENATOR && senatorInProcess == true) {
			Acquire(senatorLock);
			/* Write("%s is going outside the Passport Office because there is a Senator present.\n",
			 currentThread->getName()); */
			Wait(senatorCV, senatorLock);
			Release(senatorLock);
		}
		if (senatorInProcess == false
				|| (customers[customer].type == SENATOR
						&& currentSenator.SSN == customers[customer].SSN)) {

			if (customers[customer].picDone == false
					|| customers[customer].appDone == false) {
				picAppCustomerProcess(customer);
			}
			else if (customers[customer].appDone == true
					&& customers[customer].picDone == true
					&& customers[customer].certified == false
					&& customers[customer].earlybird == false) {
				passportCustomerProcess(customer);

			}
			else if (customers[customer].certified == true
					|| (customers[customer].earlybird == true
							&& customers[customer].gotPassport == false)) {
				cashierCustomerProcess(customer);
			}

			if (customers[customer].type != SENATOR
					&& senatorInProcess == true) {
				Acquire(senatorLock);
				Wait(senatorCV, senatorLock);
				Release(senatorLock);
			}
		}
	}

	/* Write("%s is leaving the Passport Office.\n",
	 customers[customerIndex]->name); */
	Write("Customer ", 9, ConsoleOutput);
	PrintInt(customer);
	Write(" is leaving the Passport Office\n", 32, ConsoleOutput);

	Acquire(customerCounterLock);
	customersServed++;
	Release(customerCounterLock);

	if (customers[customer].type == SENATOR && senatorInProcess == true) {
		Acquire(senatorLock);
		/* currentSenator = NULL; */
		senatorInProcess = false;
		Broadcast(senatorCV, senatorLock);
		Release(senatorLock);
		/* senatorSema.V(); */
	}

	Exit(0);
}
