#include "syscall.h"

int main() {
	int c = -1;
	int l = -1;

	Write("testcondition1: this test creates a lock and condition and calls acquire, broadcast, and release in the right order.\n", 114, ConsoleOutput);

	l = CreateLock("Lock 1", 6);
	Acquire(l);

	c = CreateCondition("Condition 1",11);
	Broadcast(c, l);

	Release(l);
	DestroyCondition(c);
	DestroyLock(l);
}
