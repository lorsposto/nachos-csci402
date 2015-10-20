#include "syscall.h"

int c, l;

void print() {
    Write("Hello world. \n", 14, ConsoleOutput);
    Acquire(l);
    Signal(c, l);
    Release(l);
    Exit(0);
}

int main() {
    Write("testlockcv3: this test creates a lock and condition and calls acquire, wait, signal and release in the right order.\n", 116, ConsoleOutput);

    l = CreateLock("Lock 1", 6);
    c = CreateCondition("Condition 1",11);

    Fork(print);

    Wait(c, l);

    DestroyCondition(c);
    DestroyLock(l);
}
