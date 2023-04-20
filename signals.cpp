#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
  std::cout << "Got Z signal\n"; // TODO: delete
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
  SmallShell& smash = SmallShell::getInstance(); // Same smash instance as in main
  std::cout << "Got C signal\n"; // TODO: delete
  pid_t fgCommand = smash.returnFgPid();
  kill(fgCommand, SIGKILL);
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

