#include "syscall.h"

int main() {
	int c = -1;
	int l = -1;
	l = CreateLock("Lock 1", 6);
	Acquire(l);

	c = CreateCondition("Condition 1",11);
	Wait(c, l);
	Signal(c, l);

	Release(l);
	DestroyCondition(c);
	DestroyLock(l);
}
