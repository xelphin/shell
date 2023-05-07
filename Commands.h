#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <chrono>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define OUTPUT_MAX_OUT (80)

class InvalidCommand : public std::exception {
public:
    const char* what() const noexcept override {
        return "This external command does not exist";
    }
};

class Command {
protected:
    bool isBackground;
    const char* cmd_line;
    char* cmd_args[80]; // array of the arguments given to Smash, but so that it functions in External Commands
    int args_count;
    // CLEAN (ignoring &)
    const char* cmd_line_clean;
    char* cmd_args_clean[80];
    int args_count_clean;
    //
public:
    Command(const char* cmd_line);
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
private:
    char* cmd_args_clean_external[82];
    bool is_alarm;
    int timeout_duration = 0;
    const char* full_timeout_str;
public:
    ExternalCommand(const char* cmd_line, bool is_alarm = false, int timeout_duration = 0, const char* full_timeout_str = "");
    virtual ~ExternalCommand() {
        int N = (this->args_count_clean);
        for (int i = 0; i < N + 2; ++i) {
            free(this->cmd_args_clean_external[i]);
        }
    }
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
private:
    std::string* lastPwd;
public:
    ChangeDirCommand(const char* cmd_line, std::string* lastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* p_jobList;
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};


class JobsList {
public:
    // JobEntry holds data about Job
    class JobEntry {
        public:
            pid_t m_pid;
            std::string m_cmd_line;
            std::time_t m_init;
            bool m_isStopped;
            bool m_isTimeout;
            int m_timeout_duration;
            JobEntry(const pid_t pid, std::string cmd_line, bool isStopped, bool isTimeout = false, int timeout_duration = 0);
            ~JobEntry() {};
    };
    std::vector<JobEntry> jobs_vector ;
    int amount_killed_but_had_alarm = 0;
public:
    JobsList();
    ~JobsList() {};
    void addJob(const pid_t pid, std::string cmd_line, bool isStopped = false, bool isTimeout = false, int duration = 0);
    void printJobsList();
    void killAllJobs();
    JobEntry * getJobById(int jobId);
    bool jobExists(int jobId, std::string& job_cmd, pid_t& job_pid, bool removeLast, bool isFgCommand);
    bool stoppedJobExists(int jobId, std::string& job_cmd, pid_t& job_pid, bool removeLast);
    bool removeJobByPID(pid_t job_pid);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    void setJobPidStopState(pid_t pid, int signal);
    bool isTimeout(pid_t pid) const;
    bool killTimeoutBecauseOfAlarm(); // only used by timeoutList
};

class JobsCommand : public BuiltInCommand {
    JobsList* p_jobList;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* p_jobList;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* p_jobList;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
    int m_duration;
    std::string m_command;
public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class ChmodCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ChmodCommand(const char* cmd_line);
    virtual ~ChmodCommand() {}
    void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    GetFileTypeCommand(const char* cmd_line);
    virtual ~GetFileTypeCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
    JobsList* p_jobList;
public:
    SetcoreCommand(const char* cmd_line, JobsList* jobs);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* p_jobList;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class SmallShell {
private:
    // Private date members
    std::string smashPrompt;
    std::string lastWorkingDirectory;
    pid_t fg_pid = getpid();
    std::string fg_cmd_line = "";
    JobsList* jobList;
    JobsList* timeoutList;
    bool killSmash = false;

    // Private functions
    void setPrompt(const std::string cmd_line);
    std::string getLastWorkingDirectory() const;
    std::string* getLastWorkingDirectoryPointer() {
        return &lastWorkingDirectory;
    }
    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char *cmd_line);
    // TODO: add extra methods as needed

    // OUR METHODS
    std::string getSmashPrompt() const;
    pid_t returnFgPid() const; 
    std::string returnFgCmdLine() const;
    void updateFgPid(const pid_t newFgPid);
    void updateFgCmdLine(const char * newFgCmdLine);
    void addJob(pid_t pid, std::string cmd_line, bool isStopped, bool isTimeout = false, int duration = 0);
    void addTimeoutToList(pid_t pid, std::string cmd_line, int duration);
    bool getKillSmash();
    void setKillSmash();
    bool pidIsTimeout(pid_t pid);
    bool giveAlarm();
    void killAllZombies();
};

#endif //SMASH_COMMAND_H_
