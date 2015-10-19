#include "syscall.h"

typedef enum { false, true } bool;

int main() {
	int option = 0;
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

	Write("\n\n", 2, ConsoleOutput);
}
