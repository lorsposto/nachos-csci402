#include "syscall.h"

int main() {
    int m, n;
    m = CreateMonitor("TestMonitor", 11, 1);
    PrintInt(m);
    Write("\n", 1, ConsoleOutput);
    SetMonitor(m, 0, 8);
    n = GetMonitor(m, 0);
    PrintInt(n);
    Write("\n", 1, ConsoleOutput);
    DestroyMonitor(m);
}
