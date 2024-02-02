#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>

#include "Commands.h"


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

//SmallShell

SmallShell::SmallShell() : last_pwd(nullptr) {
    //TODO: add your implementation
}

SmallShell::~SmallShell() {
    //TODO: add your implementation
}

std::string SmallShell::prompt_name = "smash";
pid_t SmallShell::pid = getpid();

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
        return new JobsCommand(cmd_line);}
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

Command::Command(const char *cmd_line) : cmd_line(cmd_line) {}

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

//ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}
//
//void ExternalCommand::execute() {
//    char **args = new char *[COMMAND_MAX_ARGS];
//    int cmd_arg_num = _parseCommandLine(cmd_line, args);
//    pid_t pid = fork();
//    if (pid == 0) {
//        if (execvp(args[0], args) == -1) {
//            perror("smash error: execvp failed");
//        }
//    } else if (pid < 0) {
//        perror("smash error: fork failed");
//    } else {
//        int status;
//        waitpid(pid, &status, WUNTRACED);
//    }
//    delete[] args;
//}