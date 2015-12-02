#include "syscall.h"

int main() {
    int m, n, p;
    m = CreateMonitor("TestMonitor", 11, 1);
    p = CreateMonitor("TestMonitor2", 12, 1);
    CreateMonitor("TestMonitor3", 12, 1);
    CreateMonitor("TestMonitor4", 12, 1);
    CreateMonitor("TestMonitor5", 12, 1);
    CreateMonitor("TestMonitor6", 12, 1);
    CreateMonitor("TestMonitor7", 12, 1);
    CreateMonitor("TestMonitor8", 12, 1);
    CreateMonitor("TestMonitor9", 12, 1);
    PrintInt(m);
    Write("\n", 1, ConsoleOutput);
    SetMonitor(m, 0, 8);
    n = GetMonitor(m, 0);
    PrintInt(n);
    Write("\n", 1, ConsoleOutput);
    DestroyMonitor(m);
}
