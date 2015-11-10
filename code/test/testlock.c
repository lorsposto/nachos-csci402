#include "syscall.h"

int main() {
	int l = -1;
	l = CreateLock("Lock 1", 6);
	Exit(0);
}
