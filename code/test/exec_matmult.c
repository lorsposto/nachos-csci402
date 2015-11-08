/* exec_matmult.c
 *	Simple program to test the exec system calls with matmult.
 */

#include "syscall.h"

int main() {
    Exec("../test/matmult", 15);
    Exec("../test/matmult", 15);
}
