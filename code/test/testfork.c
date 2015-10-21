/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

void print() {
	Write("Hello world. \n", 14, ConsoleOutput );
	Exit(0);
}

int main() {
  Write("testfork.c: this test forks a hello world function 5 times.\n", 60, ConsoleOutput );
  Fork(print);
  Fork(print);
  Fork(print);
  Fork(print);
  Fork(print);
  Write("Done testing Fork \n", 19, ConsoleOutput );
  Exit(0);
}
