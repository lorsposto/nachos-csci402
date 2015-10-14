/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

void print() {
	Write("Hello world. \n", 15, ConsoleOutput );
}

int main() {
  OpenFileId fd;
  int bytesread;
  char buf[20];

   /* Write("testing Exec \n", 16, ConsoleOutput );
    Exec("../test/sort", 14);
    Write("ending Exec \n", 15, ConsoleOutput ); */
  Write("Testing Fork \n", 15, ConsoleOutput );
  Fork(print);
  Write("Done testing Fork \n", 20, ConsoleOutput );
}
