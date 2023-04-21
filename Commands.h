#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <unistd.h>
#include <memory>

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
public:
    ExternalCommand(const char* cmd_line);
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
// TODO: Add your data members
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};


class JobsList {
public:
    // JobEntry holds data about Job
    class JobEntry {
            pid_t m_pid;
            const char* m_cmd_line;
            int m_Running;
            bool m_isStopped;
        public:
            JobEntry(pid_t pid, const char* cmd_line, bool isStopped );
            ~JobEntry() {};
    };
    std::vector<JobEntry> jobs_vector;
public:
    JobsList() {};
    ~JobsList() {};
    void addJob(pid_t pid, const char* cmd_line, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
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
    // TODO: Add your data members
public:
    SetcoreCommand(const char* cmd_line);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
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
    Command* cmd = nullptr;
    std::unique_ptr<JobsList> jobList;

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
    void updateFgPid(const pid_t newFgPid);
};

#endif //SMASH_COMMAND_H_
