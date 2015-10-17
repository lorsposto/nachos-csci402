#include "syscall.h"

void beCustomer() {
	Write("I am a customer.\n",17,ConsoleOutput);
}

int main() {
	int i;
	for(i=0; i < 5; i++)
		Fork(beCustomer);

}
