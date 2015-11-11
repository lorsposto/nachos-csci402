#include "syscall.h"

int main() {
	int l = -1;
	Write("testlock: this test creates a lock and calls acquire on it.\n", 60, ConsoleOutput);
	l = CreateLock("Lock 1", 6);
	Acquire(l);
	Release(l);
	DestroyLock(l);
	Exit(0);
}
