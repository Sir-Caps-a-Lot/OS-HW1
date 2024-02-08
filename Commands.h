#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <fstream>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
 public:
 
  bool background;
  bool redirect;
  char** argv;
  int fd;
  int std_fd;
  std::vector<std::string> words;
  std::string full_cmd;

  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  char** argv;
  std::string command;
  bool complex;
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand();
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
  public: 
    char ** last;
    std::string dest;
// TODO: Add your data members public:
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
  public:
  ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ChpromptCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  public:
    bool kill;

// TODO: Add your data members public:
    QuitCommand(const char* cmd_line);
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
    public:
     int job_id;
     int job_pid;
     std::string cmd_line;
     time_t start_time;
     JobEntry(int job_id, int job_pid, const char* cmd_line, time_t start_time) : job_id(job_id), job_pid(job_pid),
              cmd_line(cmd_line), start_time(start_time) {}

     void updateTime();
  };
 // TODO: Add your data members

  std::vector<JobEntry> jobs_list;
  int max_job_id;

  JobsList() : jobs_list(), max_job_id(1) {}
  ~JobsList() = default;
  void addJob(const char* cmd, int pid);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry* getJobByCmd(const char* cmd);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class AlarmList {
public:
    class AlarmEntry {
    public:
        time_t start_time;
        time_t duration;
        std::string cmd_line;
        int pid;

        AlarmEntry(time_t start_time, time_t duration, const char* cmd_line, pid_t pid) : start_time(start_time),
        duration(duration), cmd_line(cmd_line), pid(pid) {}
    };

    std::vector<AlarmEntry> alarms;

    AlarmList() : alarms() {}
    ~AlarmList() = default;
    void addAlarm(const char* cmd, int pid, time_t duration);
    void removeFinishedAlarms();
};


class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  int perm;
  bool valid;
  const char* dest_file;
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
 public:
  TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
  char* last_pwd;
  static std::string prompt;
  static JobsList jobs;
  static AlarmList alarms;
  time_t duration;

  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
