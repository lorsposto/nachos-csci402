#include "syscall.h"

int main() {
	Write("run_passport_office: executes two instances of the passport office.\n", 68, ConsoleOutput);
	Exec("../test/passport_office", 23);
	Exec("../test/passport_office", 23);
	Exit(0);
}
