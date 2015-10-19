#include "syscall.h"

typedef enum { false, true } bool;

int NUM_CUSTOMERS = 0;
int NUM_SENATORS = 0;
int NUM_PIC_CLERKS = 0;
int NUM_APP_CLERKS = 0;
int NUM_PP_CLERKS = 0;
int NUM_CASHIERS = 0;
int NUM_MANAGERS = 0;
int customersServed = 0;

bool senatorInProcess = false;
struct Customer currentSenator;

struct Clerk picClerkLines[100];
struct Clerk appClerkLines[100];
struct Clerk passportClerkLines[100];
struct Cashier cashierLines[100];

struct Manager managers[100];

struct Customer customers[100];

struct Customer {

};

struct Clerk {

};

struct Manager {

};

struct Cashier {

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

	int sen = NUM_SENATORS;

	unsigned int i;

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		struct Clerk c;
		passportClerkLines[i] = c;
		Fork(bePassportClerk);
	}

	for (i = 0; i < NUM_CASHIERS; ++i) {
		struct Cashier c;
		cashierLines[i] = c;
		Fork(beCashier);
	}

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		struct Clerk c;
		picClerkLines[i] = c;
		Fork(bePicClerk);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		struct Clerk c;
		appClerkLines[i] = c;
		Fork(beAppClerk);
	}

	NUM_CUSTOMERS += NUM_SENATORS;
	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		if (sen > 0) {
			struct Customer c;
			customers[i] = c;
			Fork(beCustomer);
			sen--;
		}
		else {
			struct Customer c;
			customers[i] = c;
			Fork(beCustomer);
		}
	}
	for (i = 0; i < NUM_MANAGERS; ++i) {
		struct Manager m;
		managers[i] = m;
		Fork(beManager);
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
