#include "syscall.h"

int main() {
    int c = -1;
    int l = -1;

    Write("testlockcv2: this test creates a lock and condition and calls acquire, signal, and release in the right order.\n", 111, ConsoleOutput);

    l = CreateLock("Lock 1", 6);
    Acquire(l);

    c = CreateCondition("Condition 1",11);
    Signal(c, l);

    Release(l);
    DestroyCondition(c);
    DestroyLock(l);
}
