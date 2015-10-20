/* testexec1.c
 *	Simple program to test the exec system calls by executing Hello World 5 times.
 */

#include "syscall.h"

int main() {
   Write("testexec1: this test executes helloworld 5 times.\n", 51, ConsoleOutput );
    Exec("../test/helloworld", 18);
    Exec("../test/helloworld", 18);
    Exec("../test/helloworld", 18);
    Exec("../test/helloworld", 18);
    Exec("../test/helloworld", 18);
}
