#include "syscall.h"

int main() {
    int c = -1;
    int l = -1;

    Write("testlockcvbad2: this test creates a lock and condition and calls broadcast without acquiring the lock.\n", 103, ConsoleOutput);

    l = CreateLock("Lock 1", 6);

    c = CreateCondition("Condition 1",11);
    Broadcast(c, l);

    DestroyCondition(c);
    DestroyLock(l);
}
