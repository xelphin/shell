#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  std::cout << "smash: got ctrl-Z\n";
  // Get current foreground command (if none, then fg_pid will be smash pid)
  SmallShell& smash = SmallShell::getInstance(); // Same smash instance as in main
  pid_t fg_pid = smash.returnFgPid();
  std::string fg_cmd_line = smash.returnFgCmdLine();

  if (fg_pid != getpid()) {
    // JOB LIST
    smash.addJob(fg_pid, fg_cmd_line, true);
    // IF there is a child (in foreground) then stop fg_pid (child)
    kill(fg_pid, SIGSTOP);
    std::cout << "smash: process " << std::to_string(fg_pid) <<" was stopped\n";

  } else {
    // TODO: Check how the "smash>" should be printed, maybe I should throw; and catch; in main instead of continuing on std::getline()
  }
  

}

void ctrlCHandler(int sig_num) {
  std::cout << "smash: got ctrl-C\n";
  // Get current foreground command (if none, then fg_pid will be smash pid)
  SmallShell& smash = SmallShell::getInstance(); // Same smash instance as in main
  pid_t fg_pid = smash.returnFgPid();

  if (fg_pid != getpid()) {
    // IF there is a child (in foreground) then kill fg_pid (child)
    kill(fg_pid, SIGKILL);
    std::cout << "smash: process " << std::to_string(fg_pid) <<" was killed\n";
  } else {
    // TODO: Check how the "smash>" should be printed, maybe I should throw; and catch; in main instead of continuing on std::getline()
  }
  
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

