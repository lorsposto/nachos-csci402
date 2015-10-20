#include "syscall.h"

int main() {
	int l = -1;
    Write("testlockbad: this test creates a lock and calls acquire and release in the wrong order.\n", 88, ConsoleOutput);
	l = CreateLock("Lock 1", 6);
	Release(l);
	Acquire(l);
	DestroyLock(l);
}
