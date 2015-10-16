/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

void print() {
	Write("Hello world. \n", 14, ConsoleOutput );
}

int main() {
  Write("Testing Fork \n", 14, ConsoleOutput );
  Fork(print);
  Write("Done testing Fork \n", 19, ConsoleOutput );
}
