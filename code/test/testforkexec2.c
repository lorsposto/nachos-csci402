/* testforkexec.c
 *	Simple program to test the exec system calls by executing
 */

#include "syscall.h"

void hello() {
   Write("Hello World\n", 17, ConsoleOutput );
   Exit(0);
}

int main() {
   Write("testforkexec2: executes matmult2, forks a hello world function, "
		   "executes the testfork user program, then executes the helloworld user program.\n", 143, ConsoleOutput );
    Exec("../test/matmult2", 16);
    Fork(hello);
    Exec("../test/testfork", 16);
    Exec("../test/helloworld", 18);

    Write("\n", 1, ConsoleOutput);
    Exit(0);
}
