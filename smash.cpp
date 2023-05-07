#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <exception>
#include "Commands.h"
#include "signals.h"
#include <string>

int main(int argc, char* argv[]) {


    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &alarmHandler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("smash error: SIGALRM failed");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(!smash.getKillSmash()) {
        std::cout << smash.getSmashPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        try {
            smash.executeCommand(cmd_line.c_str());
        } catch (const InvalidCommand& e) {
            return 127; 
        }
    }
    return 0;
}