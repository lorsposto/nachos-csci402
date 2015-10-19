#include "syscall.h"

#define NULL 0
typedef enum { false, true } bool;

unsigned int NUM_CUSTOMERS = 0;
unsigned int NUM_SENATORS = 0;
unsigned int NUM_PIC_CLERKS = 0;
unsigned int NUM_APP_CLERKS = 0;
unsigned int NUM_PP_CLERKS = 0;
unsigned int NUM_CASHIERS = 0;
unsigned int NUM_MANAGERS = 0;
unsigned int customersServed = 0;

bool senatorInProcess = false;
Customer[] currentSenator = NULL;

Clerk[] picClerkLines[100];
Clerk[] appClerkLines[100];
Clerk[] passportClerkLines[100];
Cashier[] cashierLines[100];

Manager[] managers[100];

Customer[] customers[100];

struct Customer {

	Customer(char * n, int ssn, customerType t) {

	}
};

struct Clerk {

	Clerk() {
		
	}

	Clerk(char * n, int i, clerkType t) {
		
	}
};

class Manager {

	Manager(char * n, int i) {

	}
};

struct Cashier {

	Cashier() {

	}

	Cashier(char * n, int i) {

	}
};

void bePassportClerk(int passportClerkIndex) {
	Exit(0);
}

void beCashier(int cashierIndex) {
	Exit(0);
}

void bePicClerk(int picClerkIndex) {
	Exit(0);
}

void beAppClerk(int appClerkIndex) {
	Exit(0);
}

void beCustomer(int customerIndex) {
	Exit(0);
}

void beManager(int managerIndex) {
	Exit(0);
}

int main() {

	int NUM_CUSTOMERS = 20;
	int NUM_SENATORS = 0;
	int NUM_PIC_CLERKS = 1;
	int NUM_APP_CLERKS = 1;
	int NUM_PP_CLERKS = 1;
	int NUM_CASHIERS = 1;
	int NUM_MANAGERS = 1;

	char[] name;
	unsigned int i;

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		passportClerkLines[i] = new Clerk("Passport Clerk ", i, Clerk::PP);
		name = passportClerkLines[i]->name;
		Fork((VoidFunctionPtr) bePassportClerk);
	}

	for (i = 0; i < NUM_CASHIERS; ++i) {
		cashierLines[i] = new Cashier("Cashier ", i);
		name = cashierLines[i]->name;
		Fork((VoidFunctionPtr) beCashier);
	}

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		picClerkLines[i] = new Clerk("Pic Clerk ", i, Clerk::PIC);
		name = picClerkLines[i]->name;
		Fork((VoidFunctionPtr) bePicClerk);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		appClerkLines[i] = new Clerk("Application Clerk ", i, Clerk::APP);

		name = appClerkLines[i]->name;
		t->Fork((VoidFunctionPtr) beAppClerk, i);
	}

	int sen = NUM_SENATORS;
	NUM_CUSTOMERS += NUM_SENATORS;
	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		name = new char[20];
		if (sen > 0) {
			customers[i] = new Customer(name, i, Customer::SENATOR);
			Fork((VoidFunctionPtr) beCustomer);
			sen--;
		}
		else {
			customers[i] = new Customer(name, i, Customer::REGULAR);
			Fork((VoidFunctionPtr) beCustomer);
		}
	}
	for (i = 0; i < NUM_MANAGERS; ++i) {
		managers[i] = new Manager("Manager ", i);

		name = managers[i]->name;
		Fork((VoidFunctionPtr) beManager);
	}

	/* TODO: figure out how to get user input */
	/* int option = 0;
	char c;
	bool validinput = false;

	Write("Enter a value 1-8: ", 20, ConsoleOutput);

	Read(&option, 1, ConsoleOutput);

	while (!validinput) {
		switch (option) {
			case 1:
				validinput = true;
				break;
			case 2:
				validinput = true;
				break;
			case 3:
				validinput = true;
				break;
			case 4:
				validinput = true;
				break;
			case 5:
				validinput = true;
				break;
			case 6:
				validinput = true;
				break;
			case 7:
				validinput = true;
				break;
			case 8:
				validinput = true;
				break;
			default:
				Write("Invalid input. \n", 16, ConsoleOutput);
		}
	}

	Write("\n\n", 2, ConsoleOutput); */
}
