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

typedef enum customerType {
	CUSTOMER, SENATOR
};

typedef enum {
	APP, PIC, PP
} clerkType;

typedef enum {
	AVAILABLE, BUSY, BREAK
} clerkState;

int customerIndex = 0;
int passportClerkIndex = 0;
int picClerkIndex = 0;
int appClerkIndex = 0;
struct Customer {
	int SSN;
	bool picDone;
	bool appDone;
	bool certified;
	bool gotPassport;
	bool earlybird;
	int money;
	clerkType type;
};

struct Clerk {
	int index;
	bool approved;
	int bribeLineCount;
	int regularLineCount;
	clerkType type;
	clerkState state;
	int regularLineCV;
	int bribeLineCV;
	int transactionCV;
	int transactionLock;

	int breakCV;

	int customer;
	int money;
};

struct Manager {
	int index;
	int counter;
};

struct Cashier {
	int index;
	bool approved;
	int lineCount;
	int money;
	clerkState state;
	int lineCV;
	int transactionCV;
	int transactionLock;

	int breakCV;

	int customer;
};

int Customer(int ssn) {
	customers[customerIndex].SSN = ssn;
	customers[customerIndex].picDone = false;
	customers[customerIndex].appDone = false;
	customers[customerIndex].certified = false;
	customers[customerIndex].gotPassport = false;
	customers[customerIndex].earlybird = false;
	customers[customerIndex].money = 100;
	customers[customerIndex].type = CUSTOMER;
	customerIndex++;
	return customerIndex;
}

int Senator(int ssn) {
	customers[customerIndex].SSN = ssn;
	customers[customerIndex].picDone = false;
	customers[customerIndex].appDone = false;
	customers[customerIndex].certified = false;
	customers[customerIndex].gotPassport = false;
	customers[customerIndex].earlybird = false;
	customers[customerIndex].money = 100;
	customers[customerIndex].type = SENATOR;
	customerIndex++;
	return customerIndex;
}

int AppClerk(int index) {
	appClerkLines[appClerkIndex].index = index;
	appClerkLines[appClerkIndex].approved = false;
	appClerkLines[appClerkIndex].bribeLineCount = 0;
	appClerkLines[appClerkIndex].regularLineCount = 0;
	appClerkLines[appClerkIndex].type = APP;
	appClerkLines[appClerkIndex].state = AVAILABLE;
	appClerkLines[appClerkIndex].regularLineCV = CreateCondition("AppClerkRegularCV", 17);
	appClerkLines[appClerkIndex].bribeLineCV = CreateCondition("AppClerkBribeCV", 15);
	appClerkLines[appClerkIndex].transactionCV = CreateCondition("AppClerkTransactionCV", 21);
	appClerkLines[appClerkIndex].transactionLock = CreateLock("AppClerkTransactionLock", 23);
	appClerkLines[appClerkIndex].breakCV = CreateCondition("AppClerkBreakCV", 15);
	appClerkLines[appClerkIndex].customer = -1;
	appClerkLines[appClerkIndex].money = 0;
	appClerkIndex++;
	return appClerkIndex;
}

int PicClerk(int index) {
	picClerkLines[picClerkIndex].index = index;
	picClerkLines[picClerkIndex].approved = false;
	picClerkLines[picClerkIndex].bribeLineCount = 0;
	picClerkLines[picClerkIndex].regularLineCount = 0;
	picClerkLines[picClerkIndex].type = PIC;
	picClerkLines[picClerkIndex].state = AVAILABLE;
	picClerkLines[picClerkIndex].regularLineCV = CreateCondition("PicClerkRegularCV", 17);
	picClerkLines[picClerkIndex].bribeLineCV = CreateCondition("PicClerkBribeCV", 15);
	picClerkLines[picClerkIndex].transactionCV = CreateCondition("PicClerkTransactionCV", 21);
	picClerkLines[picClerkIndex].transactionLock = CreateLock("PicClerkTransactionLock", 23);
	picClerkLines[picClerkIndex].breakCV = CreateCondition("PicClerkBreakCV", 15);
	picClerkLines[picClerkIndex].customer = -1;
	picClerkLines[picClerkIndex].money = 0;
	picClerkIndex++;
	return picClerkIndex;
}

int PassportClerk(int index) {
	passportClerkLines[passportClerkIndex].index = index;
	passportClerkLines[passportClerkIndex].approved = false;
	passportClerkLines[passportClerkIndex].bribeLineCount = 0;
	passportClerkLines[passportClerkIndex].regularLineCount = 0;
	passportClerkLines[passportClerkIndex].type = PP;
	passportClerkLines[passportClerkIndex].state = AVAILABLE;
	passportClerkLines[passportClerkIndex].regularLineCV = CreateCondition("PPClerkRegularCV", 16);
	passportClerkLines[passportClerkIndex].bribeLineCV = CreateCondition("PPClerkBribeCV", 14);
	passportClerkLines[passportClerkIndex].transactionCV = CreateCondition("PPClerkTransactionCV", 20);
	passportClerkLines[passportClerkIndex].transactionLock = CreateLock("PPClerkTransactionLock", 22);
	passportClerkLines[passportClerkIndex].breakCV = CreateCondition("PPClerkBreakCV", 14);
	passportClerkLines[passportClerkIndex].customer = -1;
	passportClerkLines[passportClerkIndex].money = 0;
	passportClerkIndex++;
	return passportClerkIndex;
}

void bePassportClerk(int passportClerkIndex) {
	Write("Hi pp\n", 6, ConsoleOutput);
	Exit(0);
}

void beCashier(int cashierIndex) {
	Exit(0);
}

void bePicClerk(int picClerkIndex) {
	Write("Hi pi\n", 6, ConsoleOutput);
	Exit(0);
}

void beAppClerk(int appClerkIndex) {
	Write("Hi ap\n", 6, ConsoleOutput);
	Exit(0);
}

void beCustomer(int customerIndex) {
	Write("Hi cs\n", 6, ConsoleOutput);
	Exit(0);
}

void beManager(int managerIndex) {
	Exit(0);
}

int main() {

	int NUM_CUSTOMERS = 10;
	int NUM_SENATORS = 0;
	int NUM_PIC_CLERKS = 1;
	int NUM_APP_CLERKS = 1;
	int NUM_PP_CLERKS = 1;
	int NUM_CASHIERS = 0;
	int NUM_MANAGERS = 0;

	int sen = NUM_SENATORS;

	unsigned int i;
	int option;
	char c;
	bool validinput;

	for (i = 0; i < NUM_PP_CLERKS; ++i) {
		PassportClerk(passportClerkIndex);
		Fork(bePassportClerk);
	}

	/*for (i = 0; i < NUM_CASHIERS; ++i) {
		struct Cashier c;
		cashierLines[i] = c;
		Fork(beCashier);
	}*/

	for (i = 0; i < NUM_PIC_CLERKS; ++i) {
		PicClerk(picClerkIndex);
		Fork(bePicClerk);
	}

	for (i = 0; i < NUM_APP_CLERKS; ++i) {
		AppClerk(appClerkIndex);
		Fork(beAppClerk);
	}

	NUM_CUSTOMERS += NUM_SENATORS;
	for (i = 0; i < NUM_CUSTOMERS; ++i) {
		if (sen > 0) {
			Senator(customerIndex);
			Fork(beCustomer);
			sen--;
		}
		else {
			Customer(customerIndex);
			Fork(beCustomer);
		}
	}

	/*for (i = 0; i < NUM_MANAGERS; ++i) {
		struct Manager m;
		managers[i] = m;
		Fork(beManager);
	}*/

	/* TODO: figure out how to get user input */
	/*option = 0;
	validinput = false;

	Write("Enter a value 1-8: ", 20, ConsoleOutput);

	Read(option, 1, ConsoleInput);

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

	Write("\n\n", 2, ConsoleOutput);*/
}
