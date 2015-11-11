#include "syscall.h"

int main() {
	int l1 = -1, l2 = -1, l3;
	Write("testlockname: this test creates three locks to show multiple locks can be created, and two locks with the same name to show that names must be unique.\n", 151, ConsoleOutput);
	l1 = CreateLock("Lock 1", 6);
	l2 = CreateLock("Lock 2", 6);
	l3 = CreateLock("Lock 1", 6);
	Write("l1 (Lock 1) has index ", 22, ConsoleOutput);
	PrintInt(l1);
	Write("\n", 1, ConsoleOutput);
	Write("l2 (Lock 2) has index ", 22, ConsoleOutput);
	PrintInt(l2);
	Write("\n", 1, ConsoleOutput);
	Write("l3 (Lock 1) has index ", 22, ConsoleOutput);
	PrintInt(l3);
	Write("\n", 1, ConsoleOutput);
	Exit(0);
}
