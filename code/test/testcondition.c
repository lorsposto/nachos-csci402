#include "syscall.h"

int main() {
	int c = -1;
	int l = -1;
	/*l = CreateLock("Lock 1", 6);
	Write("Lock ",5,ConsoleOutput);
	PrintInt(l);
	Write("\n", 1, ConsoleOutput);
	Acquire(l);
	Write("Lock acquired\n", 13, ConsoleOutput);*/

	c = CreateCondition("Condition 1",11);
	DestroyCondition(c);
	/*Write("Condition ",10,ConsoleOutput);
	PrintInt(c);
	Write("\n", 1, ConsoleOutput);
	Broadcast(c, l);*/

	/*Release(l);

	DestroyLock(l);*/
	Exit(0);
}
