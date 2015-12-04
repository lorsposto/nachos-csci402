#include "syscall.h"

int main() {
    int m, n, o, p;
    m = CreateMonitor("aaaaaaaaaaa", 11, 100);
    p = CreateMonitor("bbbbbbbbbbb", 11, 100);
    CreateMonitor("ccccccccccc", 11, 100);
    CreateMonitor("ddddddddddd", 11, 100);
    CreateMonitor("eeeeeeeeeee", 11, 100);
    CreateMonitor("fffffffffff", 11, 100);
    CreateMonitor("ggggggggggg", 11, 100);
    CreateMonitor("hhhhhhhhhhh", 11, 100);
    CreateMonitor("iiiiiiiiiii", 11, 100);
    CreateMonitor("jjjjjjjjjjj", 11, 100);
    CreateMonitor("kkkkkkkkkkk", 11, 100);
    CreateMonitor("lllllllllll", 11, 100);
    CreateMonitor("mmmmmmmmmmm", 11, 100);
    CreateMonitor("nnnnnnnnnnn", 11, 100);
    CreateMonitor("ooooooooooo", 11, 100);
    CreateMonitor("ppppppppppp", 11, 100);
    CreateMonitor("qqqqqqqqqqq", 11, 100);
    CreateMonitor("rrrrrrrrrrr", 11, 100);
    CreateMonitor("sssssssssss", 11, 100);
    CreateMonitor("ttttttttttt", 11, 100);
    CreateMonitor("uuuuuuuuuuu", 11, 100);
    CreateMonitor("vvvvvvvvvvv", 11, 100);
    CreateMonitor("wwwwwwwwwww", 11, 100);
    CreateMonitor("xxxxxxxxxxx", 11, 100);
    CreateMonitor("yyyyyyyyyyy", 11, 100);
    CreateMonitor("zzzzzzzzzzz", 11, 100);
    CreateMonitor("xxxxxxxxxxxx", 12, 100);
    CreateMonitor("xxxxxxxxxxxxx", 13, 100);
    CreateMonitor("xxxxxxxxxxxxxx", 14, 100);
    CreateMonitor("xxxxxxxxxxxxxxx", 15, 100);
    CreateMonitor("xxxxxxxxxxxxxxxx", 16, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxx", 17, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxx", 18, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxxx", 19, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxxxx", 20, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxxxxx", 21, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxxxxxx", 22, 100);
    /* The following three lines may cause seg fault! */
    /* CreateMonitor("xxxxxxxxxxxxxxxxxxxxxxx", 23, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxxxxxxxx", 24, 100);
    CreateMonitor("xxxxxxxxxxxxxxxxxxxxxxxxx", 25, 100); */
    PrintInt(m);
    Write("\n", 1, ConsoleOutput);
    SetMonitor(m, 0, 8);
    n = GetMonitor(m, 0);
    SetMonitor(p, 1, 12);
    o = GetMonitor(p, 1);
    PrintInt(n);
    Write("\n", 1, ConsoleOutput);
    PrintInt(o);
    Write("\n", 1, ConsoleOutput);
    DestroyMonitor(m);
}
