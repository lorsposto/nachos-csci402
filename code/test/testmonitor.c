#include "syscall.h"

int main() {
    int m = CreateMonitor("TestMonitor", 11, 1);

    SetMonitor(m, 0, 1);
    GetMonitor(m, 0);

}
