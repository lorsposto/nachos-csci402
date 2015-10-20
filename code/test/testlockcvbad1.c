#include "syscall.h"

int main() {
    int c = -1;
    int l = -1;

    Write("testlockcvbad1: this test creates a condition without a valid lock.\n", 68, ConsoleOutput);

    c = CreateCondition("Condition 1",11);
    Broadcast(c, l);
    DestroyCondition(c);
}
