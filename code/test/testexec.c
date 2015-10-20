/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

int main() {
   Write("testing Exec \n", 16, ConsoleOutput );
    Exec("../test/helloworld", 18);
    Exec("../test/helloworld", 18);
}
