#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction alrm;
    alrm.sa_handler = alarmHandler;
    alrm.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &alrm, NULL);

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
    std::cout << smash.prompt << "> ";
    std::string cmd_line;
    std::getline(std::cin, cmd_line);
    smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}