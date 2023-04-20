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
  std::cout << "smash: got ctrl-C\n";
  // Get current foreground command (if none, then fgCommand will be smash pid)
  SmallShell& smash = SmallShell::getInstance(); // Same smash instance as in main
  pid_t fgCommand = smash.returnFgPid();

  if (fgCommand != getpid()) {
    // IF there is a child (in foreground) then kill fgCommand (child)
    kill(fgCommand, SIGKILL);
    std::cout << "smash: process " << std::to_string(fgCommand) <<" was killed\n";
  } else {
    // TODO: Check how the "smash>" should be printed, maybe I should throw; and catch; in main instead of continuing on std::getline()
  }
  
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

