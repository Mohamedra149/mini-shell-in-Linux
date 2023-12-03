#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    //TODO: setup sig alarm handler
    struct sigaction action;
    memset (&action, '\0', sizeof(action));
    action.sa_sigaction = alarmHandler;
    action.sa_flags = SA_SIGINFO | SA_RESTART;
    if (sigaction(SIGALRM, &action, nullptr) < 0) {
        perror ("smash error: failed to set alarm handler");
    }
    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.shell_name +"> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}