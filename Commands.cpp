#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <exception>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

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

bool _isComplex(const char* cmd_line) {
    const string str(cmd_line);
    int N = str.length();
    for (int i=0; i< N; i++) {
        if (str[i] == '*' || str[i] == '?') return true;
    }
    return false;
}

std::string _findXthWord(std::vector<std::string>& vector, int x)
{
    int count = 0;
    for(std::vector<std::string>::const_iterator it = vector.begin(); it != vector.end(); ++it) {
        if (count == x) {
            return *it;
        }
        count++;
    }
    return "";
}

void _chkStatus(int pid, int stat)
{
    if (WIFEXITED(stat)) {
        int exit_status = WEXITSTATUS(stat);
        if (exit_status == 127) {
            // Child returned 127 (INVALID COMMAND)
            //std::cout << "Child returned 127\n";
        } else {
            // Child returned a non-127 status
            //std::cout << "Child returned " << exit_status << "\n";
        }
    }  else if (WIFSTOPPED(stat)) {
        // Child process stopped by signal
        //std::cout << "Child process " << pid << " stopped by signal" << std::endl;
    }  else if (WIFSIGNALED(stat)) {
        int signal_number = WTERMSIG(stat);
        if (signal_number == SIGINT) {
            //std::cout << "Child was terminated by a SIGINT signal\n";
        } else {
            //std::cout << "Child was terminated by signal " << signal_number << "\n";
        }
    } else {
        //std::cerr << "Child terminated abnormally\n";
    }
}

int _fillVectorWithStrings(const std::string str_full, std::vector<std::string>& vector)
{
    std::string str = _trim(string(str_full));
    if (str=="") return 0;
    // Variables
    int startWord = 0;
    int lengthCurrWord = 0;
    int countWords = 0;
    bool amInBlank = false;
    // Get words
    int N = str.length();
    for (int i=0; i < N; i++) {
        // case: reached end of a word because got to a space character
        if (!amInBlank && str[i]==' ') {
            // add word
            vector.push_back(_trim(str.substr(startWord,lengthCurrWord)));
            // update
            countWords++;
            lengthCurrWord = 0;
            amInBlank = true;
        }
            // case: am at section with lots of spaces/blank
        else if (amInBlank && str[i]==' ') {
            // do nothing
        }
            // case: was in Blank but now got to word
        else if (amInBlank && str[i]!=' ') {
            amInBlank = false;
            startWord = i;
            lengthCurrWord = 1;
        }
            // case: currently inside of word
        else if(!amInBlank  && str[i]!= ' ') {
            lengthCurrWord++;
        }

    }
    // case: reached end of word because reached end of string
    countWords++;
    vector.push_back(_trim(str.substr(startWord,lengthCurrWord)));
    return vector.size();
}


void _updateCommandForExternal(char* cmd_args_array[])
{
    // Depending on Command add necessary prefix
    if (cmd_args_array[0] != NULL) {
        // Make string of updated command
        std::string updatedCommand = "/bin/"; // TODO: have more types
        updatedCommand.append(cmd_args_array[0]);
        // Update cmd_args_array[0] to match string
        delete[] cmd_args_array[0];
        cmd_args_array[0] = new char[updatedCommand.size() + 1];
        strcpy(cmd_args_array[0], updatedCommand.c_str());
    }
}

void _updateCommandForExternalComplex(const char* cmd_line, char** cmd_args_external)
{
    // In External Command, you want the first two arguments to be "/bin/bash" and "-c" and then all the other arguments from cmd_line
    std::string strBash = "/bin/bash";
    std::string strC = "-c";
    //
    cmd_args_external[0] = (char*)malloc(strBash.length()+1);
    memset(cmd_args_external[0], 0, strBash.length()+1);
    strcpy(cmd_args_external[0], strBash.c_str());
    //
    cmd_args_external[1] = (char*)malloc(strC.length()+1);
    memset(cmd_args_external[1], 0, strC.length()+1);
    strcpy(cmd_args_external[1], strC.c_str());

    // Copy Rest
    int i = 2;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        cmd_args_external[i] = (char*)malloc(s.length()+1);
        memset(cmd_args_external[i], 0, s.length()+1);
        strcpy(cmd_args_external[i], s.c_str());
        cmd_args_external[++i] = NULL;
    }
}

bool _isValidFgBgInput( char* cmd_args_array[],  int args_count, std::string type)
{
    if ( args_count< 1 || args_count> 2) {
        std::cerr << "smash error: "<< type <<": invalid arguments\n";
        return false;
    }
    bool isNegResolved = true;
    if (args_count == 2) {
        for (int i = 0; cmd_args_array[1][i] != '\0'; i++) {
            if (i == 0 && cmd_args_array[1][i] == '-') {
                isNegResolved = false;
                continue;
            }
            if (!std::isdigit(cmd_args_array[1][i])) {
                std::cerr << "smash error: "<< type <<": invalid arguments\n";
                return false;
            }
            isNegResolved = true;
        }
    }
    if (!isNegResolved) {
        std::cerr << "smash error: "<< type <<": invalid arguments\n";
        return false;
    }
    return true;
}

bool _isValidKillSyntax(char* cmd_args_array[],  int args_count, int& signal, int& jobId)
{
    if (args_count < 3 || args_count > 3) return false;
    if (strcmp(cmd_args_array[0], "kill") != 0) return false;

    // CHECK valid signal syntax
    int count = 0;
    for(; cmd_args_array[1][count] != '\0'; count++) {
        char chr = cmd_args_array[1][count];
        if (count == 0 && cmd_args_array[1][0] != '-') return false;
        if(count != 0 && !isdigit(chr)) return false;
    }
    if (count < 2) return false;
    signal = atoi(cmd_args_array[1] + 1);

    // CHECK valid id syntax
    count = 0;
    for(; cmd_args_array[2][count] != '\0'; count++) {
        char chr = cmd_args_array[2][count];
        if(!isdigit(chr)) return false;
    }
    if (count <1) return false;
    jobId = atoi(cmd_args_array[2]);

    // okay

    return true;
}


// -------------------------------
// ------- COMMAND CLASSES -------
// -------------------------------

// TODO: Add your implementation for classes in Commands.h

Command::Command(const char* cmd_line) : cmd_line(cmd_line), args_count(0) {
    this->isBackground = _isBackgroundComamnd(cmd_line);

    // WITH & version
    this->args_count = _parseCommandLine(this->cmd_line, this->cmd_args);
    // Set last to be NULL
    this->cmd_args[this->args_count + 1] = NULL;

    // CLEAN VERSION
    string cmd_s = _trim(string(cmd_line));
    if (cmd_s != "") {
        // Clean (without &) versions of cmd_s and firstWord
        char* clean_ptr = new char[cmd_s.length() + 1];
        strcpy(clean_ptr, cmd_s.c_str());
        _removeBackgroundSign(clean_ptr);
        this->cmd_line_clean = clean_ptr;
        this->args_count_clean = _parseCommandLine(this->cmd_line_clean, this->cmd_args_clean);
        // Set last to be NULL
        this->cmd_args_clean[this->args_count_clean + 1] = NULL;
    }
}

Command::~Command() {
    for (int i = 0; i < (this->args_count); ++i) {
        free(this->cmd_args[i]);
    }
    for (int i = 0; i < (this->args_count_clean); ++i) {
        free(this->cmd_args_clean[i]);
    }
    delete[] cmd_line_clean;
}

// -------------------------------
// ------- JOB LIST --------------
// -------------------------------

JobsList::JobEntry::JobEntry(const pid_t pid, std::string cmd_line, bool isStopped ) : m_pid(pid), m_cmd_line(cmd_line), m_isStopped(isStopped)  {
    m_init = std::time(nullptr);
}

JobsList::JobsList() : jobs_vector()
{}

void JobsList::addJob(const pid_t pid, std::string cmd_line, bool isStopped) 
{
    (this->jobs_vector).push_back(JobEntry(pid, cmd_line, isStopped));
}

void JobsList::printJobsList()
{
    this->killAllZombies();
    int count = 1;
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        std::cout << "[" << std::to_string(count) <<"]" << it->m_cmd_line << ":" << std::to_string(it->m_pid)<< " ";
        std::time_t time_now = std::time(nullptr);
        int elapsed_seconds = std::difftime(time_now, it->m_init);
        std::cout << std::to_string(elapsed_seconds) << " secs";
        if (it->m_isStopped) {
            std::cout << " (stopped)\n";
        } else {
            std::cout << std::endl;
        }
        count++;
    }
}

void JobsList::killAllZombies()
{
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end();) {
        int status;
        pid_t pid = it->m_pid;

        // If zombie then free
        int res = waitpid(pid, &status, WNOHANG);
        if (res == -1) {
            // Error occurred
            perror("smash error: waitpid failed");
            it++;
        }
        else if (res == 0) {
            // Job is not a zombie
            it++;
        } else if (WIFSTOPPED(status)) {
            // Child process stopped by signal
            // std::cout << "Child process " << res << " stopped by signal, not zombie" << std::endl;
        }
        else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Child process has exited or was terminated by a signal
            // Free its PID
            // std::cout << "killing zombie: " << it->m_cmd_line << std::endl;
            int status;
            if (waitpid(pid, &status, WUNTRACED) == -1) perror("smash error: waitpid failed");
            it = jobs_vector.erase(it);
        }
        else {
            // unknown
            it++;
        }
    }
}

void JobsList::killAllJobs()
{
    // kill all zombies before we start
    this->killAllZombies();

    // PRINT JOBS
    int vectorSize = ( this->jobs_vector).size();
    std::cout << "smash: sending SIGKILL signal to "<< std::to_string(vectorSize) <<" jobs:\n";
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        std::cout <<  std::to_string(it->m_pid)<< ":" << it->m_cmd_line << std::endl;
    }

    // KILL ALL JOBS
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        if (kill(it->m_pid, SIGKILL) == -1) perror("smash error: kill failed");
    }
}

bool JobsList::jobExists(int jobId, std::string& job_cmd, pid_t& job_pid, bool removeLast, bool isFgCommand)
{
    int indexJobId = jobId-1;
    this->killAllZombies();
    if (jobs_vector.empty() && removeLast) {
        if (isFgCommand) std::cerr << "smash error: fg: jobs list is empty\n";
        return false;
    }
    int vectorSize = ( this->jobs_vector).size();
    if (indexJobId >= 0 && indexJobId < vectorSize && !removeLast) {
        job_cmd = jobs_vector[indexJobId].m_cmd_line;
        job_pid = jobs_vector[indexJobId].m_pid;
        return true;
    }
    else if (removeLast) {
        job_cmd = jobs_vector.back().m_cmd_line;
        job_pid = jobs_vector.back().m_pid;
        return true;
    }
    if (indexJobId < 0 || indexJobId >= vectorSize) {
        if (isFgCommand) std::cerr << "smash error: fg: job-id "<< std::to_string(jobId) <<" does not exist\n";

    }
    return false;
}

bool JobsList::stoppedJobExists(int jobId, std::string& job_cmd, pid_t& job_pid, bool removeLast)
{
    int indexJobId = jobId-1;
    this->killAllZombies();
    if (jobs_vector.empty() && removeLast) {
        std::cerr << "smash error: bg: there is no stopped jobs to resume\n";
        return false;
    }
    int vectorSize = ( this->jobs_vector).size();
    // REMOVE LAST
    if (removeLast) {
        for (std::vector<JobEntry>::reverse_iterator it = jobs_vector.rbegin(); it != jobs_vector.rend(); ++it){
            if (it->m_isStopped) {
                it->m_isStopped = false;
                job_cmd = it->m_cmd_line;
                job_pid = it->m_pid;
                return true;
            }
        }
        std::cerr << "smash error: bg: there is no stopped jobs to resume\n";
        return false;
    }
    // REMOVE AT JOB_ID
    if (indexJobId >= 0 && indexJobId < vectorSize) {
        // Job exists
        if (jobs_vector[indexJobId].m_isStopped != true) {
            // Job is running
            std::cerr << "smash error: bg: job-id "<< std::to_string(jobId) <<" is already running in the background\n";
            return false;
        }
        // Job is stopped
        jobs_vector[indexJobId].m_isStopped = false;
        job_cmd = jobs_vector[indexJobId].m_cmd_line;
        job_pid = jobs_vector[indexJobId].m_pid;
        return true;
    }
    // Job doesn't exist
    std::cerr << "smash error: bg: job-id "<< std::to_string(jobId) <<" does not exist\n";
    return false;
}

bool JobsList::removeJobByPID(pid_t job_pid)
{
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end();){
        if (it->m_pid == job_pid) {
            it = jobs_vector.erase(it);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}

void JobsList::setJobPidStopState(pid_t pid, int signal)
{
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        if (it->m_pid == pid) {
            if (signal == 18) { // SIGCONT
                it->m_isStopped = false;
            } else if (signal == 19) { // SIGSTOP
                it->m_isStopped = true;
            }
            // Zombies are killed in above functions
        }
    }
}
// --------------------------------
// ------- EXTERNAL COMMAND -------
// --------------------------------

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {
    _updateCommandForExternalComplex(cmd_line_clean, cmd_args_clean_external);
}

void ExternalCommand::execute()
{
    int stat;
    // TODO: The following is a basic implementation for commands like "date", "ls", "ls -a" that run in foreground, do the rest
    if (this->args_count < 1) {
        // std::cout << " Too few words\n"; // TODO: Erase, figure out what should actually be printed (although should never get here)
        return;
    }

    // FORK
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    }

    // CHILD
    else if (pid == 0) {
        setpgrp();
        if (_isComplex(this->cmd_line)) {
            // Is COMPLEX
            if (execv(cmd_args_clean_external[0], cmd_args_clean_external) == -1) perror("smash error: execv failed");
        } else {
            // Is SIMPLE
            if (execvp(this->cmd_args_clean[0], this->cmd_args_clean) == -1) perror("smash error: execvp failed");
        }
        // INVALID COMMAND
        // std::cerr << "Error executing command\n";
        // If has &, it was added wrongfully to joblist, so need to remove TODO 
        // (but also, maybe invalid commands isn't something we need to support)
        throw InvalidCommand();
    }

    // PARENT
    else {
        SmallShell& smash = SmallShell::getInstance();

        // BACKGROUND
        if (this->isBackground) {
            // Notice: If invalid background command, then instantly waitpid() finishes and removes it TODO
            smash.addJob(pid, cmd_line, false); 
        } 

        // FOREGROUND
        else {
            // Update Foreground PID to be that of Child
            smash.updateFgPid(pid);
            smash.updateFgCmdLine(this->cmd_line);
            // Wait for child to finish/get signal
            if (waitpid(pid, &stat, WUNTRACED ) < 0) {
                perror("smash error: wait failed");
            } else {
                //std::cout << "Child: " << cmd_line_clean << ", finished (external)\n"; // TODO: delete
                _chkStatus(pid, stat);
                //std::cout << "my status: " << std::to_string(stat) << std::endl;
                if (WEXITSTATUS(stat) == 127) {
                    // Valid Command
                    // std::cout << "INVALID COMMAND\n"; // TODO: Edit this

                }
            }
        }

        // Update Foreground PID of Smash
        smash.updateFgPid(getpid()); // child is dead, so change back foreground to mean smash process again
        // If in background, then don't wait!
    }
}

// ----------------------------------
// ------- BUILT IN COMMANDS -------
// -----------------------------------

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

// KILL COMMAND

KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), p_jobList(jobs) {}

void KillCommand::KillCommand::execute()
{
    if (p_jobList == nullptr) { // TODO: erase this, or make a throw
        //std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    int signal, jobId;
    if (!_isValidKillSyntax(this->cmd_args_clean, this->args_count_clean, signal, jobId )) {
        std::cerr << "smash error: kill: invalid arguments\n";
        return;
    }
    // Check Job ID exists
    std::string job_cmd;
    pid_t job_pid;
    bool exists = false;
    exists = p_jobList->jobExists(jobId, job_cmd, job_pid, false, false);
    if (!exists) {
        std::cerr << "smash error: kill: job-id "<< std::to_string(jobId) <<" does not exist\n";
        return;
    }
    // std::cout << "My arguments: signal: "<< std::to_string(signal) << " , jobId: " << std::to_string(jobId) << std::endl;
    // Send signal to Job
    std::cout << "signal number "<< std::to_string(signal) << " was sent to pid "<< std::to_string(job_pid) <<"\n";
    int result = kill(job_pid, signal);
    if(result == -1) {
        perror("smash error: kill failed");
    } else {
        // UPDATE JOB LIST (SIGSTOP/SIGCONT)
        p_jobList->setJobPidStopState(job_pid, signal);
    }
}

// QUIT COMMAND

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), p_jobList(jobs) {}

void QuitCommand::QuitCommand::execute()
{
    // Checks valid format
    if (p_jobList == nullptr) { // TODO: erase this, or make a throw
        // std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    if (this->args_count_clean < 1 ) return;
    if (strcmp(this->cmd_args_clean[0], "quit") != 0) return;
    if (this->args_count_clean > 1 && strcmp(this->cmd_args_clean[1], "kill") != 0) return;
    SmallShell& smash = SmallShell::getInstance();
    // QUIT
    if (this->args_count_clean == 1) {
        smash.setKillSmash();
        return;
    }
    // QUIT KILL
    p_jobList->killAllJobs();
    smash.setKillSmash();
}

// FG COMMAND

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), p_jobList(jobs) {}

void ForegroundCommand::execute()
{
    // Checks valid format
    if (p_jobList == nullptr) { // TODO: erase this, or make a throw
        // std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    if (!_isValidFgBgInput(cmd_args_clean, args_count_clean, "fg")) return;

    // Check Job ID exists
    std::string job_cmd;
    pid_t job_pid;
    bool exists = false;
    if (args_count_clean != 1) {
        // "fg <int>"
        exists = p_jobList->jobExists(std::stoi(cmd_args_clean[1]), job_cmd, job_pid, false, true);
    } else {
        // "fg"
        exists = p_jobList->jobExists(-1, job_cmd, job_pid, true, true);
    }
    if (!exists) return;

    // WAIT
    std::cout << job_cmd << ":" << std::to_string(job_pid) << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    int stat;
    // Update Foreground PID to be that of Child
    smash.updateFgPid(job_pid);
    smash.updateFgCmdLine(job_cmd.c_str());
    // send SIGCONT
    if (kill(job_pid, SIGCONT) == -1) perror("smash error: kill failed");
    // Wait for child to finish/get signal
    if (waitpid(job_pid, &stat, WUNTRACED ) < 0) {
        perror("smash error: wait failed");
    } else {
        // WAIT FINISHED
        _chkStatus(job_pid, stat);
        // std::cout << "my status: " << std::to_string(stat) << std::endl;
        if (WEXITSTATUS(stat) == 127) {
            // Invalid Command
            // std::cout << "INVALID COMMAND\n"; // TODO: Edit this
        }
    }
        
    // Update Foreground PID of Smash
    smash.updateFgPid(getpid()); // child is dead, so change back foreground to mean smash process again
    // If in background, then don't wait!

    // REMOVE
    p_jobList->removeJobByPID(job_pid);
}

// BACKGROUND COMMAND

BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), p_jobList(jobs) {}

void BackgroundCommand::execute()
{
    // Checks valid format
    if (p_jobList == nullptr) { // TODO: erase this, or make a throw
        // std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    if (!_isValidFgBgInput(cmd_args_clean, args_count_clean, "bg")) return;

    // Check Job ID exists and is stopped
    std::string job_cmd;
    pid_t job_pid;
    bool exists = false;
    if (args_count_clean != 1) {
        // "bg <int>"
        exists = p_jobList->stoppedJobExists(std::stoi(cmd_args_clean[1]), job_cmd, job_pid, false);
    } else {
        // "bg"
        exists = p_jobList->stoppedJobExists(-1, job_cmd, job_pid, true);
    }
    if (!exists) return;

    // PRINT command
    std::cout << job_cmd << ":" << std::to_string(job_pid) << std::endl;

    // CONTINUE stopped process
    if (kill(job_pid, SIGCONT) == -1) perror("smash error: kill failed");
}

// JOBS COMMAND

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), p_jobList(jobs) {}

void JobsCommand::execute()
{
    if (p_jobList == nullptr) { // TODO: erase this, or make a throw
        // std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    this->p_jobList->printJobsList();
}

// CHANGE_DIR COMMAND

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, std::string* lastPwd) : BuiltInCommand(cmd_line), lastPwd(lastPwd) {}

void ChangeDirCommand::execute()
{
    // Get current directory
    std::string currDir = "/";
    char cwd[OUTPUT_MAX_OUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currDir = cwd;
    } else {
        perror("smash error: getcwd failed");
    }

    // See if there's a third word (illegal)
    if (this->args_count > 2) {
        std::cerr << "smash error: cd: too many arguments\n";
        return;
    }
    if (this->args_count < 2) {
        std::cerr << "smash error: cd: too few arguments\n"; // TODO: ?
        return;
    }
    // See if there's only one word->"cd" (illegal?) // TODO: what to do if only get "cd" (or "cd&")?
    std::string secondWord = _trim(string(cmd_args[1]));

    // See if special: second word is "-"
    if (secondWord == "-") {
        if (*lastPwd == "") {
            std::cerr << "smash error: cd: OLDPWD not set\n";
            return;
        }
        if (chdir((*lastPwd).c_str()) != 0) {
            // Not successful
            std::perror("smash error: cd failed"); 
            return;
        }
    } else {
        // Normal: cd <path>
        if (chdir(secondWord.c_str()) != 0) {
            // Not successful
            std::perror("smash error: cd failed"); 
            return;
        }
    }

    // Update last working directory
    *lastPwd = currDir;

    // TODO: Check if instead I have to continue going back in cd history (I think above is what they wanted)
}

// GET_CURR_DIR COMMAND

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute()
{
    std::string text = "";

    // Get path
    char cwd[OUTPUT_MAX_OUT]; // TODO: check that the pwd can't in fact exceed 80 characters (asked in Piazza)
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        text += cwd;
    } else {
        perror("smash error: getcwd failed");
    }
    text += "\n";

    // Print
    std::cout << text;
}

// SHOW_PID COMMAND

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute()
{
    std::string text = "smash pid is ";
    // Get pid : TODO: Check that this is actually the pid() that they want
    pid_t pid = getpid();
    if (pid == -1) perror("smash error: getpid failed");
    std::string pidStr = std::to_string(pid);
    text += pidStr;
    text += "\n";
    // Print
    std::cout << text;
}

// --------------------------
// ------- SMALL SHELL -------
// --------------------------

SmallShell::SmallShell() {
// TODO: add your implementation
    this->smashPrompt = "smash> ";
    this->lastWorkingDirectory = ""; // uninitialized
    this->jobList = new JobsList();
}

SmallShell::~SmallShell() {
    if (this->jobList!=nullptr) delete jobList;
}

std::string SmallShell::getSmashPrompt() const
{
    return this->smashPrompt;
}


pid_t SmallShell::returnFgPid() const
{
    return fg_pid;
}

std::string SmallShell::returnFgCmdLine() const
{
    return fg_cmd_line;
}

void SmallShell::updateFgPid(const pid_t newFgPid) 
{
    this->fg_pid = newFgPid;
}

void SmallShell::updateFgCmdLine(const char * newFgCmdLine)
{
    this->fg_cmd_line = newFgCmdLine;
}


void SmallShell::setPrompt(const std::string cmd_line)
{
    // Turn to char* so that can apply _removeBackgroundSign()
    char* clean_ptr = new char[cmd_line.length() + 1];
    strcpy(clean_ptr, cmd_line.c_str());
    _removeBackgroundSign(clean_ptr);
    std::string clean_str(clean_ptr);

    // make vector of words in prompt
    std::vector<std::string> tmp_vector;
    _fillVectorWithStrings(clean_str, tmp_vector);

    // Find second word and update smash prompt
    std::string secondWord = _findXthWord(tmp_vector, 1);
    if (secondWord == "") {
        this->smashPrompt = "smash> ";
    } else {
        this->smashPrompt = secondWord + "> ";
    }

    // Clear memory
    delete[] clean_ptr;
}

std::string SmallShell::getLastWorkingDirectory() const
{
    return this->lastWorkingDirectory;
}

void SmallShell::addJob(pid_t pid, const std::string cmd_line, bool isStopped)
{
    if (jobList == nullptr) { // TODO: erase this, or make a throw
        // std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    jobList->addJob(pid, cmd_line, isStopped);
}

bool SmallShell::getKillSmash()
{
    return this->killSmash;
}

void SmallShell::setKillSmash()
{
    this->killSmash = true;
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char *cmd_line) {

    // EXTRACT INPUTS

    // Get first word (command)
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    

    if (cmd_s == "") return nullptr; // TODO: Not sure what they want us to do in this case, but this works for now,update if needed

    // Clean (without &) versions of cmd_s and firstWord
    char* clean_ptr = new char[cmd_s.length() + 1];
    strcpy(clean_ptr, cmd_s.c_str());
    _removeBackgroundSign(clean_ptr);
    std::string cmd_s_clean(clean_ptr);
    delete[] clean_ptr;
    std::string firstWord_clean = cmd_s.substr(0, cmd_s_clean.find_first_of(" \n")); // note: chprompt& hello, shouldn't work

    // CHOOSE COMMAND

    if (firstWord_clean == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord_clean == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord_clean == "chprompt") {
        SmallShell::setPrompt(cmd_s);
        return nullptr;
    }
    else if (firstWord_clean == "cd") {
        return new ChangeDirCommand(cmd_s_clean.c_str(), this->getLastWorkingDirectoryPointer());
    }
    else if (firstWord_clean == "jobs") {
        return new JobsCommand(cmd_s_clean.c_str(), this->jobList);
    }
    else if (firstWord_clean == "fg") {
        return new ForegroundCommand(cmd_line, this->jobList);
    }
    else if (firstWord_clean == "bg") {
        return new BackgroundCommand(cmd_line, this->jobList);
    }
    else if (firstWord_clean == "quit") {
        return new QuitCommand(cmd_line, this->jobList);
    }
    else if (firstWord_clean == "kill") {
        return new KillCommand(cmd_line, this->jobList);
    }
        // TODO: Continue with more commands here

    else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

// -------------------------------
// ------- SHELL EXECUTION -------
// -------------------------------

void SmallShell::executeCommand(const char *cmd_line) {
    this->cmd = CreateCommand(cmd_line);
    if (this->cmd!=nullptr) {
        try {
            this->cmd->execute();
            delete cmd;
        } catch (const InvalidCommand & e) {
            if (this->cmd!=nullptr) delete cmd;
            throw InvalidCommand();
        } 
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


