#include "syscall.h"

int main() {
	Write("run_passport_office: executes the passport office.\n", 51, ConsoleOutput);
	Exec("../test/ppo_customer", 20);
	Exec("../test/ppo_appclerk", 20);
	Exit(0);
}
