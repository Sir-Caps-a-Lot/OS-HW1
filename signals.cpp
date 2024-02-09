#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if(!(smash.fg_pid==0)) {
    kill(smash.fg_pid , SIGINT);
    cout << "smash: process " << smash.fg_pid << " was killed" << endl;
    smash.fg_pid=0;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
  cout << "smash: got an alarm" << endl;
  SmallShell& smash = SmallShell::getInstance();
  smash.alarms.removeFinishedAlarms();
}

