#include "syscall.h"

int main() {
	int l = -1;
	Write("xxx", 3, ConsoleOutput);
	l = CreateLock("Lock 1", 6);
	Exit(0);
}
