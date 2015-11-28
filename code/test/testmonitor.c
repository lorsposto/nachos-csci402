#include "syscall.h"

int main() {
    int m;
    m = CreateMonitor("TestMonitor", 11, 1);
    PrintInt(m);
    SetMonitor(m, 0, 1);
    GetMonitor(m, 0);
    DestroyMonitor(m);
}
