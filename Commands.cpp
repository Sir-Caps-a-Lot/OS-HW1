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


#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(' ');
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(' ');
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
  return str[str.find_last_not_of(' ')] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(' ');
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
  cmd_line[str.find_last_not_of(' ', idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

//Command::Command(const char* cmd_line) {
//    this->std_fd = dup(1);
//    string temp = string(cmd_line);
//    stringstream ss(temp);
//    string buffer;
//    while (ss >> buffer) {
//        this->words.push_back(buffer);
//    }
//
//    // Find if need to redirect output
//    this->redirect = this->words.size() > 2;
//    const char* dest_file;
//    int wtype;
//    if (this->redirect) {
//        dest_file = this->words[this->words.size()-1].c_str();
//        if (this->words[this->words.size()-2].compare(">") == 0) {
//            wtype = O_WRONLY|O_CREAT|O_TRUNC;
//        }
//        else if (this->words[this->words.size()-2].compare(">>") == 0) {
//            wtype = O_WRONLY|O_APPEND|O_CREAT;
//        }
//        else {
//            this->redirect = false;
//        }
//    }
//
//    // Redirect stdout
//    if (this->redirect) {
//        int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
//        this->fd = open(dest_file, wtype, perm);
//        if (this->fd == -1) {
//            perror("smash error: open: failed");
//            this->redirect = false;
//        }
//        else {
//            close(1);
//            if (dup2(this->fd, 1)==-1) {
//                perror("smash error: dup2: failed");
//                close(this->fd);
//                dup2(this->std_fd, 1);
//                this->redirect = false;
//            }
//        }
//    }
//
//    // Background check
//    std::string last = this->words[this->words.size() - 1];
//    this->background = *(last.end()-1) == '&';
//    if (this->background) {
//        if (last.compare("&")==0) {
//            this->words.pop_back();
//        }
//        else {
//            this->words[this->words.size() - 1] = last.substr(0, last.size()-1); // remove it from command
//        }
//    }
//}

Command::Command(const char* cmd_line) : cmd_line(cmd_line) {}

//SmallShell

SmallShell::SmallShell() : last_pwd(nullptr), curr_cmd() {
    //TODO: add your implementation
}

SmallShell::~SmallShell() {
    //TODO: add your implementation
}

std::string SmallShell::prompt_name = "smash";
pid_t SmallShell::pid = getpid();
JobsList SmallShell::jobs = JobsList();

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {

    char** args = new char*[COMMAND_MAX_ARGS];
    int cmd_arg_num = _parseCommandLine(cmd_line, args);
    cmd_arg_num +=0;
    string firstWord = args[0];
    if (firstWord == "chprompt") {
        return new ChpromptCommand(cmd_line);
    } else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    } else if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line, &last_pwd);
    } else if (firstWord == "jobs") {
        return new JobsCommand(cmd_line, &jobs);}
//    } else if (firstWord == "fg") {
//        return new ForegroundCommand(cmd_line);
//    } else if (firstWord == "quit") {
//        return new QuitCommand(cmd_line);
//    } else if (firstWord == "kill") {
//        return new KillCommand(cmd_line);
//
//    } else {
//        return new ExternalCommand(cmd_line);
//    }
    return nullptr;
}



void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  delete cmd;

}

//JobsList

JobsList::JobEntry::JobEntry(int job_id, pid_t job_pid, time_t start_time, bool is_stopped, const char* cmd_line)
        : job_id(job_id), job_pid(job_pid), start_time(start_time), is_stopped(is_stopped), cmd_line(cmd_line) {}

JobsList::JobsList() : jobs_list(), max_job_id(1){}

void JobsList::addJob(Command* cmd, bool is_stopped) {
    int job_id = max_job_id;
    max_job_id++;
    pid_t job_pid = SmallShell::pid;
    time_t start_time = time(nullptr);
    string cmd_line = cmd->get_cmd_line();
    JobEntry job_entry(job_id, job_pid, start_time, is_stopped, cmd_line.c_str());
    jobs_list.push_back(job_entry);
}

void JobsList::printJobsList() {
    for (auto job : jobs_list) {
        cout << "[" << job.job_id << "] " << job.cmd_line << " : " << job.job_pid << " " << difftime(time(nullptr), job.start_time) << " secs";
        if (job.is_stopped) {
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    for (auto job : jobs_list) {
        kill(job.job_pid, SIGKILL);
    }
}

void JobsList::removeFinishedJobs() {
    for (auto job = jobs_list.begin(); job != jobs_list.end(); job++) {
        int status;
        if (waitpid(job->job_pid, &status, WNOHANG) != 0) {
            jobs_list.erase(job);
        }
    }
}

void JobsList::removeJobById(int jobId) {
    for (auto job = jobs_list.begin(); job != jobs_list.end(); job++) {
        if (job->job_id == jobId) {
            jobs_list.erase(job);
            return;
        }
    }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for (auto job = jobs_list.begin(); job != jobs_list.end(); job++) {
        if (job->job_id == jobId) {
            return &(*job);
        }
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
    if (jobs_list.empty()) {
        return nullptr;
    }
    *lastJobId = jobs_list.back().job_id;
    return &jobs_list.back();
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {
    for (auto job = jobs_list.rbegin(); job != jobs_list.rend(); job++) {
        if (job->is_stopped) {
            *jobId = job->job_id;
            return &(*job);
        }
    }
    return nullptr;
}



const char* Command::get_cmd_line() const {
    return cmd_line;
}


BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
    string cmd_s(cmd_line);
    if (_isBackgroundComamnd(cmd_line)) {
        char cmd_line_copy[COMMAND_ARGS_MAX_LENGTH];
        strcpy(cmd_line_copy, cmd_s.c_str());
        _removeBackgroundSign(cmd_line_copy);
        cmd_s = cmd_line_copy;
    }
    char *cmd_copy = (char *) malloc(sizeof(char) * (cmd_s.length() + 1));
    strcpy(cmd_copy, cmd_s.c_str());
    this->cmd_line = cmd_copy;
}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS];
    int cmd_arg_num = _parseCommandLine(cmd_line, args);
    if (cmd_arg_num == 1) {
        SmallShell::prompt_name = "smash";
    } else if (cmd_arg_num == 2) {
        SmallShell::prompt_name = args[1];
    }
    delete[] args;
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << SmallShell::getInstance().pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    long max_path = pathconf(".", _PC_PATH_MAX);
    char *path = (char *) malloc(sizeof(char) * max_path);
    path = getcwd(path, max_path);
    cout << path << endl;
    free(path);
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **last_pwd) : BuiltInCommand(cmd_line) , plastPwd(last_pwd) {}

void ChangeDirCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS];
    int cmd_arg_num = _parseCommandLine(cmd_line, args);
    long max_path = pathconf(".", _PC_PATH_MAX);
    char *path = (char *) malloc(sizeof(char) * max_path);
    getcwd(path, max_path);

    if(cmd_arg_num > 2){
        cerr << "smash error: cd: too many arguments" << endl;
    } else if (cmd_arg_num == 1) {
        //TODO: check if we need to change to home directory
        //NO ARGUMENTS
    } else if (cmd_arg_num == 2) {
        if (strcmp(args[1], "-") == 0) {
            if (*plastPwd == nullptr) {
                cerr << "smash error: cd: OLDPWD not set" << endl;
            } else {
                if(chdir(*plastPwd) == -1){
                    cerr << "smash error: cd: chdir failed" << endl;
                    return;
                } else {
                    if (plastPwd != nullptr) {
                        free(*plastPwd);
                    }
                    *plastPwd = path;
                }
            }
        } else if (strcmp(args[1], "..") == 0) {
            if(chdir("..") == -1){
                cerr << "smash error: cd: chdir failed" << endl;
                return;
            } else {
                if (plastPwd != nullptr) {
                    free(*plastPwd);
                }
                *plastPwd = path;
            }

        } else {
            if (chdir(args[1]) == -1) {
                perror("smash error: chdir failed");
            } else {
                if (plastPwd != nullptr) {
                    free(*plastPwd);
                }
                *plastPwd = path;
            }
        }
    }
    delete[] args;
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line) {}

void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();

    smash.jobs.removeFinishedJobs();

    for (auto job = smash.jobs.jobs_list.begin(); job != smash.jobs.jobs_list.end()  ;job++) {

        cout << "[" << job->job_id << "] " << job->cmd_line << " : " << job->job_pid
        << " " << difftime(time(nullptr), job->start_time) << " secs";
        if (job->is_stopped) {
            cout << " (stopped)";
        }
        cout << endl;
    }
}


//
//ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {
//    // Get full command (no redirection)
//    std::string full_cmd = string(cmd_line);
//    if (this->redirect) {
//        full_cmd = full_cmd.substr(0, full_cmd.find(" >"));
//
//        // Find if need /bin/bash
//        this->complex = false;
//        this->complex |= full_cmd.find('?') != std::string::npos;
//        this->complex |= full_cmd.find('*') != std::string::npos;
//        if (this->complex) {
//            this->command = string("/bin/bash");
//        } else {
//            this->command = this->words[0];
//        }
//
//        // Find the argvs
//        int argc = !this->complex ? this->words.size() - 2 * this->redirect : 3;
//        if (!this->complex) { // normal external
//            int argc = this->words.size() - 2 * this->redirect;
//            this->argv = new char *[argc + 1];
//            for (int i = 0; i < argc; i++) {
//                this->argv[i] = new char[this->words[i].size() + 1];
//                strcpy(this->argv[i], this->words[i].c_str());
//            }
//        } else {
//            this->argv = new char *[4]; // bash -c "command"
//            this->argv[0] = new char[10]; // "/bin/bash" + null terminator
//            strcpy(this->argv[0], "/bin/bash");
//            this->argv[1] = new char[3]; // "-c" + null terminator
//            strcpy(this->argv[1], "-c");
//            this->argv[2] = new char[full_cmd.size() + 1];
//            strcpy(this->argv[2], full_cmd.c_str());
//        }
//        this->argv[argc] = nullptr;
//    }
//}
//
//ExternalCommand::~ExternalCommand() {
//    int argc = (!this->complex) ? (this->words.size() - 2 * this->redirect) : 3;
//    for (int i=0; i<argc; i++) {
//        delete[] this->argv[i];
//    }
//    delete[] this->argv;
//}

//void ExternalCommand::execute() {
//    SmallShell &smash = SmallShell::getInstance();
//    pid_t pid = fork();
//    if (pid == -1) {
//        perror("smash error: fork failed");
//        return;
//    }
//    if (pid == 0) {
//        if (this->redirect) {
//            int fd = open(this->words[this->words.size() - 1].c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
//            if (fd == -1) {
//                perror("smash error: open failed");
//                exit(1);
//            }
//            if (dup2(fd, 1) == -1) {
//                perror("smash error: dup2 failed");
//                exit(1);
//            }
//            close(fd);
//        }
//        if (this->complex) {
//            execvp(this->command.c_str(), this->argv);
//        } else {
//            execvp(this->argv[0], this->argv);
//        }
//        perror("smash error: execvp failed");
//        exit(1);
//    } else {
//        if (this->background) {
//            smash.jobs.addJob(this, false);
//        } else {
//            smash.fg_pid = pid;
//            smash.jobs.addJob(this, true);
//            waitpid(pid, nullptr, WUNTRACED);
//            smash.fg_pid = 0;
//        }
//    }
//}
//
////void ExternalCommand::execute() {
////    pid_t p = fork();
////    if (p == -1) { // failed
////        perror("smash error: fork: failed");
////    } // parent
////    else if (p==0) {
////        execvp(this->command.c_str(), this->argv);
////        perror("smash error: execvp: failed");
////        exit(1);
////    }
////    else {
////        if (!this->background) {
////            waitpid(p, nullptr, 0);
////        }
////        else {
////            // add to jobs list
////            SmallShell &smash = SmallShell::getInstance();
////            smash.jobs.addJob(this, false);
////        }
////    }
////}
////

