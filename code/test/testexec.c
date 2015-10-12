/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

int main() {
  OpenFileId fd;
  int bytesread;
  char buf[20];

    Write("testing Exec \n", 16, ConsoleOutput );
    Exec("sort.c", 6);
}
