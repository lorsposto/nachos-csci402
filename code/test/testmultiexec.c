/* testmultiexec.c
 *	Simple program to test multiple exec system calls
 */

#include "syscall.h"

int main() {
  OpenFileId fd;
  int bytesread;
  char buf[20];

   Write("testing Exec 1 \n", 18, ConsoleOutput );
   Exec("../test/sort", 14);

   Write("testing Exec 2 \n", 18, ConsoleOutput );
   Exec("../test/helloworld", 20);
}