#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if(!smash.jobs.jobs_list.empty()) {
        int process_num = smash.jobs.jobs_list.front().job_pid;
        kill(process_num , SIGINT);
        cout << "smash: process " << process_num << " was killed" << endl;
    }
}


void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    cout << "smash: got an alarm" << endl;
    SmallShell& shell = SmallShell::getInstance();

}

