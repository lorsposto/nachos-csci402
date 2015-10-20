/* testforkexec.c
 *	Simple program to test the exec system calls by executing
 */

#include "syscall.h"

void hello() {
   Write("Hello World\n", 12, ConsoleOutput );
   Exit(0);
}

int main() {
   Write("testforkexec1: this test executes helloworld 2 times, matmult2 2 times, and forks a helloworld function 2 times.\n", 113, ConsoleOutput );
    Exec("../test/matmult2", 16);
    Fork(hello);
    Exec("../test/helloworld", 18);
    Exec("../test/matmult2", 16);
    Exec("../test/helloworld", 18);
    Fork(hello);

    Write("\n", 1, ConsoleOutput);
    Exit(0);
}
