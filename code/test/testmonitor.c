#include "syscall.h"

int m;

void doGet() {
	Write("Get ", 4, ConsoleOutput);
	PrintInt(m);
	Write("\n", 1, ConsoleOutput);
	GetMonitor(m, 0);
}

void doSet() {
	SetMonitor(m, 0, 1);
}

int main() {
    m = -1;

    m = CreateMonitor("TestMonitor", 11, 1);

    Fork(doGet);
    Fork(doSet);
}
