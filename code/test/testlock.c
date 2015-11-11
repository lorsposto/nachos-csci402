#include "syscall.h"

int main() {
	int l = -1;
	l = CreateLock("Lock 1", 6);
	Acquire(l);
	Exit(0);
}
