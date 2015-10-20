/* testexec.c
 *	Simple program to test the exec system calls
 */

#include "syscall.h"

int main() {
  int r;
  r = Rand();
  PrintInt(r);
}
