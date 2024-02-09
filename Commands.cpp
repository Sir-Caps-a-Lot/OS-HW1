#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cstd::cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cstd::cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#define WHITESPACE " \n\r\t\f\v"
#define CMD_MAX_LEN 200
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//********************************** Command ************************************
Command::Command(const char* cmd_line) : redirect_failed(false), cmd_argc(0), full_cmd(cmd_line) {
  this->std_fd = dup(1);
  this->timed = false;
  stringstream ss(this->full_cmd);
  string buffer;
  while (ss >> buffer) {
    this->words.push_back(buffer);
  }

  // Find if need to redirect output
  bool find_file = false;
  int wtype;
  for (auto word : this->words) {
    this->cmd_argc++;
    int cmd_pos = 0;
    if (!find_file) {// still need to find out if redirect is needed
      if (word.find(">") == std::string::npos)
        continue;
      bool overwrite = (word.find(">>") == std::string::npos) ? true : false;
      wtype = O_WRONLY|O_CREAT;
      wtype |= (overwrite) ? O_TRUNC : O_APPEND;
      find_file = true;
      cmd_pos = word.find(">") + !overwrite + 1;
      if ((int)word.size() == cmd_pos) {
        continue;
      }
    }
    if (find_file) {// can still be reach after the first block
      int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
      const char* dest_file = word.substr(cmd_pos).c_str();
      this->fd = open(dest_file, wtype, perm);
      if (this->fd == -1) {
        perror("smash error: open: failed");
        this->redirect = false;
        this->redirect_failed = true;
      }
      else {
        close(1);
        if (dup2(this->fd, 1)==-1) {
          perror("smash error: dup2: failed");
          close(this->fd);
          dup2(this->std_fd, 1);
          this->redirect = false;
          this->redirect_failed = true;
        }
        else {
          this->redirect = true;
        }
      }
      break;
    }
  }
  
  // Background check
  if (this->words.size() !=0) {
    std::string last = this->words[this->words.size() - 1];
    this->background = *(last.end()-1) == '&';
    if (this->background) {
      if (last.compare("&")==0) {
        this->words.pop_back();
      }
      else {
        this->words[this->words.size() - 1] = last.substr(0, last.size()-1); // remove it from command
      }
    }
  }
  else {
    this->background = false;
  }
}

Command::~Command() {
  if (this->redirect) {
    close(this->fd);
    dup2(this->std_fd, 1);
  }
}

//*****************************ChangeDirCommand**********************************
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line) {
  this->dest = this->words[1];
  this->last = plastPwd;
}

void ChangeDirCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  char* current = getcwd(NULL, 0);
  if (this->words.size() > 2)
    std::cerr << "smash error: cd: too many arguments\n";
  else if (this->dest.compare("-")==0) {
    if (string(*(this->last)).compare("-")==0) {
      std::cerr << "smash error: cd: OLDPWD not set\n";
    }
    else if (chdir(*(this->last)) == -1)
      perror("smash error: chdir: failed");
    else {
      delete *(this->last);
      *(this->last) = current;
    }
  }
  else if (chdir(this->dest.c_str()) == -1)
    perror("smash error: chdir: failed");
  else {
    delete *(this->last);
    *(this->last) = current;
  }
}

//**********************************ChmodCommand*********************************
ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  try {
    if (this->words.size() == 3) {
      this->dest_file = this->words[2].c_str();
      this->perm = std::stoi(this->words[1], 0, 8);
      this->valid = true;
    }
    else {
      this->valid = false;
    }
  }
  catch(...) {
    this->valid = false;
  }
}

void ChmodCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  if (!this->valid) {
    std::cerr << "smash error: chmod: invalid arguments\n";
  }
  else if (chmod(this->dest_file, this->perm) == -1) {
    perror("smash error: chmod: failed");
  }
}

//***********************************QuitCommand*********************************
QuitCommand::QuitCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  this->kill = false;
  for (std::string word : this->words) {
    this->kill |= (word.compare("kill") == 0);
  }
}

void QuitCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  if (this->kill) {
    SmallShell::jobs.killAllJobs();
  }
  exit(0);
}

//**********************************ShowPidCommand********************************
void ShowPidCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  int p=getpid();
  if (p==-1) {
    perror("smash error: getpid: failed");
  }
  else {
    std::cout << "smash pid is " << getpid() << "\n";
  }
}

//**********************************GetCurrDirCommand******************************
void GetCurrDirCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  const char* cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    perror("smash error: getcwd: failed");
  }
  else {
    std::cout << cwd << "\n";
    delete cwd;
  }
}

//**********************************ExternalCommand**********************************
int SmallShell::fg_pid;
AlarmList SmallShell::alarms;
ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line){
  // Get full command (no redirection)
  std::string cmd = string(cmd_line);
  if (this->redirect) 
    cmd = cmd.substr(0, cmd.find(">"));

  // Find if need /bin/bash
  this->complex = false;
  this->complex |= cmd.find('?') != std::string::npos;
  this->complex |= cmd.find('*') != std::string::npos;

  // Find the argvs
  if (!this->complex) { // normal external
    this->argv = new char*[this->cmd_argc+1];
    for (int i=0; i < this->cmd_argc; i++) {
      std::string word = this->words[i];
      if (this->words[i].find(">")!=std::string::npos)
        word = word.substr(0, this->words[i].find(">"));
      this->argv[i] = new char[word.size()+1];
      strcpy(this->argv[i], word.c_str());
    }
  }
  else {
    this->argv = new char*[4]; // bash -c "command"
    this->argv[0] = new char[10]; // "/bin/bash" + null terminator
    strcpy(this->argv[0], "/bin/bash");
    this->argv[1] = new char[3]; // "-c" + null terminator
    strcpy(this->argv[1], "-c");
    this->argv[2] = new char[cmd.size()+1];
    strcpy(this->argv[2], cmd.c_str());
  }
  if (this->complex) {
    this->command = string("/bin/bash");
  }
  else if(this->words.size()!=0){
    this->command = this->argv[0];
  }
  else {
    this->command = "";
  }
  this->argv[this->cmd_argc] = nullptr;
}

void ExternalCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  if (this->words.size()==0) { // no command to do
    return;
  }
  int p=fork();
  if (p == -1) { // failed
    perror("smash error: fork: failed");
  }
  else if (p==0) { // child
    setpgrp();
    execvp(this->command.c_str(), this->argv);
    perror("smash error: execvp: failed");
    exit(1);
  }
  else { // parent
    if (this->timed) {
      auto last_alrm = SmallShell::alarms.alarms.back();
      last_alrm.pid=p;
      SmallShell::alarms.alarms.back()=last_alrm;
    }
    if (!this->background) {
      SmallShell::fg_pid = p;
      waitpid(p, nullptr, WUNTRACED);
      SmallShell::fg_pid = 0;
    }
    else {
      SmallShell::jobs.addJob(this->full_cmd.c_str(), p);
    }
  }
}

ExternalCommand::~ExternalCommand() {
  for (int i=0; i<this->cmd_argc; i++) {
    delete[] this->argv[i];
  }
  if (this->cmd_argc > 0)
    delete[] this->argv;
}

//**********************************ChpromptCommand**********************************
std::string SmallShell::prompt;
void ChpromptCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  if (this->words.size() == 1) {
    SmallShell::prompt = "smash";
  }
  else {
    SmallShell::prompt = this->words[1];
  }
}

//**********************************JobsList**********************************
JobsList SmallShell::jobs;
void JobsList::addJob(const char* cmd, int pid) {
  this->removeFinishedJobs();
  JobEntry job_entry(this->max_job_id++, pid, cmd);
  this->jobs_list.push_back(job_entry);
}

void JobsList::printJobsList() {
  this->removeFinishedJobs();
  for (auto job : this->jobs_list) {
      std::cout << "[" << job.job_id << "] " << job.cmd_line << endl;
  }
}

void JobsList::killAllJobs() { // this is for the "quit kill" command, not "kill" command
  std::cout << "smash: sending SIGKILL signal to " << this->jobs_list.size() <<" jobs:\n";
  for (auto job : this->jobs_list) {
    if (kill(job.job_pid, SIGKILL) == 1) {
      perror("smash error: kill failed");
    }
    else {
      std::cout << job.job_pid << ": " << job.cmd_line << endl;
    }
  }
}

void JobsList::removeFinishedJobs() { // Remove finished processes
  for (auto job = this->jobs_list.begin(); job != this->jobs_list.end(); ) {
    int status;
    if (waitpid(job->job_pid, &status, WNOHANG | WUNTRACED) != 0) {
        this->jobs_list.erase(job);
    } else {
        job++;
    }
  }
  if (this->jobs_list.size() == 0)
    this->max_job_id = 1;
  else {
    this->max_job_id = this->jobs_list.back().job_id + 1;
  }
}

void JobsList::removeJobById(int jobId) { // Used for fg command
  for (auto job = this->jobs_list.begin(); job != this->jobs_list.end();) {
    if (job->job_id == jobId) {
      jobs_list.erase(job);
      return;
    }
    else {
      job++;
    }
  }
  if (this->jobs_list.size() == 0)
    this->max_job_id = 1;
  else {
    this->max_job_id = this->jobs_list.back().job_id + 1;
  }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) { // used for fg command
  for (auto job = jobs_list.begin(); job != jobs_list.end(); job++) {
      if (job->job_id == jobId) {
          return &(*job);
      }
  }
  return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) { // used for fg command with no arguments
  if (this->jobs_list.empty()) {
      return nullptr;
  }
  *lastJobId = this->jobs_list.back().job_id;
  return &jobs_list.back();
}

// **********************************AlarmList**********************************
void AlarmList::addAlarm(const char* cmd, int pid, time_t duration) {
    AlarmEntry alarm_entry(time(nullptr), duration, cmd, pid);
    this->alarms.push_back(alarm_entry);
}

void AlarmList::removeFinishedAlarms() {
  int min = -1;
  int pid = 0;
  auto remove_index = this->alarms.begin();
  for (auto alarm = this->alarms.begin(); alarm != this->alarms.end(); alarm++) {
    time_t passed = time(nullptr) - alarm->start_time; 
    if ((passed >= alarm->duration && passed-alarm->duration < min) || min == -1) {
      min = passed-alarm->duration;
      pid = alarm->pid;
      remove_index = alarm;
    }
  }
  if (pid>0) {
    kill(pid, SIGKILL);
    std::cout << "smash: timeout "<<(*remove_index).duration << " " << (*remove_index).cmd_line << " timed out!" << std::endl;
  }
  this->alarms.erase(remove_index);
}

//**********************************JobsCommand**********************************
void JobsCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  SmallShell::jobs.printJobsList();
}

//**********************************ForegroundCommand**********************************
void ForegroundCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  if (this->words.size() == 1) { // Wait for last
    int jid;
    auto job = SmallShell::jobs.getLastJob(&jid);
    if (job == nullptr) {
      std::cerr << "smash error: fg: jobs list is empty\n";
    }
    else {
      SmallShell::jobs.removeJobById(jid);
      std::cout << job->cmd_line << " " << job->job_pid << std::endl;
    }
  }
  else if (this->words.size() > 2) { // too much parameters
    std::cerr << "smash error: fg: invalid arguments\n";
  }
  else {
    try {
      int jid = std::stoi(words[1]);
      if (std::to_string(jid).size() != words[1].size()) {
        throw nullptr; // write error in catch
      }
      auto job = SmallShell::jobs.getJobById(jid);
      if (job == nullptr) { // does not exist
        std::cerr << "smash error: fg: job-id " << jid <<" does not exist\n";
      }
      else {
        std::cout << job->cmd_line << " " << job->job_pid << std::endl;
        SmallShell::fg_pid = job->job_pid;
        SmallShell::jobs.removeJobById(jid);
        waitpid(SmallShell::fg_pid, nullptr, WUNTRACED);
        SmallShell::fg_pid = 0;
      }
    }
    catch(...) { // second parameter is not a number
      std::cerr << "smash error: fg: invalid arguments\n";
    }
  }
}

//**********************************KillCommand**********************************
void KillCommand::execute() {
  if (this->redirect_failed) {
    return;
  }
  if (this->words.size() != 3) {
    std::cerr << "smash error: kill: invalid arguments\n";
  }
  else {
    try {
      if (this->words[1][0] != '-') {
        throw nullptr;
      }
      int signal = std::stoi(this->words[1].substr(1));
      int jid = std::stoi(this->words[2]);
      if (std::to_string(jid).size() != this->words[2].size() || 
          std::to_string(signal).size() != this->words[1].size() - 1) {
        throw nullptr;
      }
      auto job = SmallShell::jobs.getJobById(jid);
      if (job == nullptr) {
        std::cerr << "smash error: kill: job-id " << jid <<" does not exist\n";
      }
      else if (kill(job->job_pid, signal)==1) {
        perror("smash error: kill failed");
      }
      else {
        std::cout << "signal number " << signal << " was sent to pid " << job->job_pid << std::endl;
      }
    }
    catch(...) {
      std::cerr << "smash error: kill: invalid arguments\n";
    }
  }
}

//**********************************TimeoutCommand**********************************
TimeoutCommand::TimeoutCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
}
void TimeoutCommand::execute() {
  if (this->words.size() < 3) {
      std::cerr << "smash error: timeout: invalid arguments\n";
  } 
  else {
    try {
      int duration = std::stoi(this->words[1]);
      if (std::to_string(duration).size() != this->words[1].size()) {
        throw nullptr;
      }
      std::string cmd = "";
      for (int i = 2; i < (int)this->words.size(); i++) {
        cmd += this->words[i];
        if (i != (int)this->words.size() - 1) {
          cmd += " ";
        }
      }
      if (this->background) {
        cmd += "&";
      }
      SmallShell &smash = SmallShell::getInstance();
      alarm(duration);
      smash.alarms.addAlarm(cmd.c_str(), 0, duration);
      Command* com = smash.CreateCommand(cmd.c_str());
      com->timed=true;
      com->full_cmd = this->full_cmd;
      com->execute();
      delete com;      
    }
    catch (...) {
      std::cerr << "smash error: timeout: invalid arguments\n";
    }
  }
}

//**********************************SmallShell**********************************
SmallShell::SmallShell() {
  this->fg_pid = 0;
  this->last_pwd = new char('-');
  this->prompt = "smash";
}

SmallShell::~SmallShell() {
  delete this->last_pwd;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE));

  if (firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0) {
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0 || firstWord.compare("cd&") == 0) {
      return new ChangeDirCommand(cmd_line, &(this->last_pwd));
  }
  else if (firstWord.compare("quit") == 0 ||firstWord.compare("quit&") == 0) {
    return new QuitCommand(cmd_line);
  }
  else if (firstWord.compare("chmod") == 0 || firstWord.compare("chmod&") == 0) {
    return new ChmodCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0) {
    return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0) {
    return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0 || firstWord.compare("kill&") == 0){
    return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("timeout") == 0 || firstWord.compare("timeout&") == 0) {
    return new TimeoutCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
}

void SmallShell::executeCommand(const char *cmd_line) {
  this->fg_pid=0;
  this->jobs.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  delete cmd;
}