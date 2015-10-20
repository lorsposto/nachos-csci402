/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

int main() {
	int r;
	r = Rand();
	PrintInt(r);
	Write("\n", 1, ConsoleOutput);
	r = Rand();
	PrintInt(r);
	Write("\n", 1, ConsoleOutput);
	r = Rand();
	PrintInt(r);
	Write("\n", 1, ConsoleOutput);
	r = Rand();
	PrintInt(r);
	Write("\n", 1, ConsoleOutput);
}
