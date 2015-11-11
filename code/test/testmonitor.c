#include "syscall.h"

int m;

void doGet() {
	Write("Get ", 4, ConsoleOutput);
	PrintInt(m);
	Write("\n", 1, ConsoleOutput);
	GetMonitor(m);
}

void doSet() {
	SetMonitor(m, 1);
}

int main() {
    int c = -1;
    int l = -1;
    m = -1;

    l = CreateLock("Lock 1", 6);
    c = CreateCondition("Condition 1",11);
    m = CreateMonitor(l, c, 1);

    Fork(doGet);
    Fork(doSet);
}
