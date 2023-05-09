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

bool _isCharArrAPosNumber(const char* arr, int& num)
{
    int count = 0;
    for(; arr[count] != '\0'; count++) {
        char chr = arr[count];
        if(!isdigit(chr)) return false;
    }
    if (arr[0] == '\0') return false;
    num = stoi(arr);
    return true;
}

bool _isCharArrANumber(const char* arr, int& num)
{
    if (arr[0] == '\0') return false;
    bool isNegResolved = true;
    for (int i = 0; arr[i] != '\0'; i++) {
        if (i == 0 && arr[i] == '-') {
            isNegResolved = false;
            continue;
        }
        if (!std::isdigit(arr[i])) {
            return false;
        }
        isNegResolved = true;
    }
    if (!isNegResolved) return false;
    num = stoi(arr);
    return true;
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
        std::string updatedCommand = "/bin/"; 
        updatedCommand.append(cmd_args_array[0]);
        // Update cmd_args_array[0] to match string
        delete[] cmd_args_array[0];
        cmd_args_array[0] = new char[updatedCommand.size() + 1];
        strcpy(cmd_args_array[0], updatedCommand.c_str());
    }
}


void _parseTimeoutCommand(const char* cmd_line, std::string& cmd_line_new_cmd, int& duration)
{

    // When given "timeout <duration> <command>" returns "<command>"

    std::string cmd_s = _trim(string(cmd_line));
    int first_space = cmd_s.find_first_of(" ");
    cmd_s = _trim(cmd_s.substr(first_space, cmd_s.length() - first_space));  // second word onwards
    first_space = cmd_s.find_first_of(" "); // end of second word

    std::string second_word = cmd_s.substr(0, first_space);
    duration = stoi(second_word);

    cmd_s = _trim(cmd_s.substr(first_space));  // third word onwards
    cmd_line_new_cmd = cmd_s;
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
    if (!_isCharArrANumber(cmd_args_array[2], jobId)) {
        return false;
    }

    // okay

    return true;
}



bool _isComplexRedirection(const char* cmd_line)
{
    std::string str(cmd_line);
    size_t index = str.find('>');
    if (index != std::string::npos) {
        if (index< str.length() -1) {
            if (str[index+1] == '>') return true;
        }
    } else {
        std::cout << "SHOULDN'T GET HERE!" << std::endl;
    }
    return false;
}

bool _isErrPipe(const char* cmd_line)
{
    std::string str(cmd_line);
    size_t index = str.find('|');
    if (index != std::string::npos) {
        if (index< str.length() -1) {
            if (str[index+1] == '&') return true;
        }
    } else {
        std::cout << "SHOULDN'T GET HERE!" << std::endl;
    }
    return false;
}

bool _cleanCharArray(char arr[])
{
    int firstNonSpace = -1;
    int lastNonSpace = -1;
    //
    for (int i = 0; arr[i] != '\0'; i++) {
        if (arr[i] != ' ' && firstNonSpace == -1) {
            firstNonSpace = i;
        }
        if (arr[i] != ' ') {
            lastNonSpace = i;
        }
    }
    if (firstNonSpace == -1 || lastNonSpace < firstNonSpace) return false;
    //
    for (int i = 0; i< lastNonSpace - firstNonSpace +1; i++) {
        arr[i] = arr[firstNonSpace + i];
    }
    arr[ lastNonSpace - firstNonSpace +1] = '\0';
    return true;

}

bool _splitSidesIntoTwoCharArrays(char leftPart[], char rightPart[], const char* cmd_line)
{
    if (strlen(cmd_line) > 80 || strlen(cmd_line) < 3) {
        return false;
    }
    // Find seperator index
    std::string str(cmd_line);
    if (str[0] == '>' || str[0] == '|') return false;
    size_t indexSep = str.find('>');
    if (indexSep == std::string::npos) {
        indexSep = str.find('|');
        if (indexSep == std::string::npos) {
            std::cout << "SHOULDN'T GET HERE!" << std::endl;
            return false; 
        }
    }
    if (indexSep > 80) {
        std::cout << "SHOULDN'T GET HERE!" << std::endl;
        return false;
    }
    // Copy left part
    strncpy(leftPart, cmd_line, indexSep);
    leftPart[indexSep] = '\0';
    // Update sepIndex (if necessary)
    indexSep++;
    if (  indexSep < strlen(cmd_line) -1  && str[indexSep] == '>') 
        indexSep++;
    else if (indexSep < strlen(cmd_line) -1 && str[indexSep] == '&') 
        indexSep++;

    if (indexSep == strlen(cmd_line)) return false;
    strcpy(rightPart, &cmd_line[indexSep]);
    if (!_cleanCharArray(leftPart) || !_cleanCharArray(rightPart)) return false;
    return true;
}





// -------------------------------
// ------- COMMAND CLASSES -------
// -------------------------------

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

JobsList::JobEntry::JobEntry(int jobId, const pid_t pid, std::string cmd_line, bool isStopped , bool isTimeout, int timeout_duration) 
: m_pid(pid), m_cmd_line(cmd_line), m_isStopped(isStopped), m_isTimeout(isTimeout),  m_timeout_duration(timeout_duration), m_job_id(jobId) {
    m_init = std::time(nullptr);
    // std::cout << "Added new job entry: jobId: " << (m_job_id) << ". Is stopped: " << (m_isStopped) << std::endl;
}

JobsList::JobsList() : jobs_vector(), job_id_count(1)
{}

void JobsList::addJob(const pid_t pid, std::string cmd_line, int old_job_id, bool isStopped, bool isTimeout, int duration) 
{
    // Clean
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies();

    // Get max jobId currently in use
    int max_jobId_currently_in_use = 0;
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        if (it->m_job_id > max_jobId_currently_in_use) {
            max_jobId_currently_in_use = it->m_job_id;
        }
    }
    this->job_id_count = max_jobId_currently_in_use + 1;

    // Add job to jobList
    if (old_job_id < 1) {
        (this->jobs_vector).push_back(JobEntry(this->job_id_count, pid, cmd_line, isStopped, isTimeout, duration));
    } else {
        // Job already existed (use old_job_id)
        (this->jobs_vector).push_back(JobEntry(old_job_id, pid, cmd_line, isStopped, isTimeout, duration));
        this->sortVectorByJobId();
    }
    
}

void JobsList::printJobsList()
{
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies();
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        std::cout << "[" << std::to_string(it->m_job_id) <<"] " << it->m_cmd_line << " : " << std::to_string(it->m_pid)<< " ";
        std::time_t time_now = std::time(nullptr);
        int elapsed_seconds = std::difftime(time_now, it->m_init);
        std::cout << std::to_string(elapsed_seconds) << " secs";
        if (it->m_isStopped) {
            std::cout << " (stopped)\n";
        } else {
            std::cout << std::endl;
        }
    }
}

void SmallShell::killAllZombies()
{
    for (std::vector<JobsList::JobEntry>::iterator it = jobList->jobs_vector.begin(); it != jobList->jobs_vector.end();) {
        int status;
        pid_t pid = it->m_pid;

        // If zombie then free
        int res = waitpid(pid, &status, WNOHANG);
        if (res == -1) {
            // Error occurred
            // perror("smash error: waitpid failed");
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

            // /// // CORRECTED BUT CHECK
            int status;
            if (waitpid(pid, &status, WUNTRACED) == -1) {
                // perror("smash error: waitpid failed");
            }

            // ERASE from timeoutList
            timeoutList->removeJobByPID(it->m_pid);
            // ERASE from jobList
            it = jobList->jobs_vector.erase(it);
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
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies();

    // PRINT JOBS
    int vectorSize = ( this->jobs_vector).size();
    std::cout << "smash: sending SIGKILL signal to "<< std::to_string(vectorSize) <<" jobs:\n";
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        std::cout <<  std::to_string(it->m_pid)<< ": " << it->m_cmd_line << std::endl;
    }

    // KILL ALL JOBS
    for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
        if (kill(it->m_pid, SIGKILL) == -1) perror("smash error: kill failed");
    }
}

bool JobsList::jobExists(int jobId, std::string& job_cmd, pid_t& job_pid, bool removeLast, bool isFgCommand, int& found_job_id)
{
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies();
    if (jobs_vector.empty() && removeLast) {
        if (isFgCommand) std::cerr << "smash error: fg: jobs list is empty\n";
        return false;
    }
    if (!removeLast) {
        for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
            if (it->m_job_id == jobId) {
                job_cmd = it->m_cmd_line;
                job_pid = it->m_pid;
                found_job_id = it->m_job_id;
                return true;
            }
        }
        if (isFgCommand) std::cerr << "smash error: fg: job-id "<< std::to_string(jobId) <<" does not exist\n";
        return false;
    }
    if (removeLast) {
        job_cmd = jobs_vector.back().m_cmd_line;
        job_pid = jobs_vector.back().m_pid;
        found_job_id = jobs_vector.back().m_job_id;
        return true;
    }
    return false;
}

bool JobsList::stoppedJobExists(int jobId, std::string& job_cmd, pid_t& job_pid, bool removeLast, int& found_job_id)
{
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies();
    if (jobs_vector.empty() && removeLast) {
        std::cerr << "smash error: bg: there is no stopped jobs to resume\n";
        return false;
    }
    // REMOVE LAST
    if (removeLast) {
        for (std::vector<JobEntry>::reverse_iterator it = jobs_vector.rbegin(); it != jobs_vector.rend(); ++it){
            if (it->m_isStopped) {
                it->m_isStopped = false;
                job_cmd = it->m_cmd_line;
                job_pid = it->m_pid;
                found_job_id = it->m_job_id;
                return true;
            }
        }
        std::cerr << "smash error: bg: there is no stopped jobs to resume\n";
        return false;
    }
    // REMOVE AT JOB_ID
    if (!removeLast) {
        for (std::vector<JobEntry>::iterator it = jobs_vector.begin(); it != jobs_vector.end(); ++it){
            if (it->m_job_id == jobId) {
                // Job exists
                if (it->m_isStopped != true) {
                    // Job is running
                    std::cerr << "smash error: bg: job-id "<< std::to_string(jobId) <<" is already running in the background\n";
                    return false;
                }
                // Job is stopped
                it->m_isStopped = false;
                job_cmd = it->m_cmd_line;
                job_pid = it->m_pid;
                found_job_id = it->m_job_id;
                return true;
            }
        }
        // Job doesn't exist
        std::cerr << "smash error: bg: job-id "<< std::to_string(jobId) <<" does not exist\n";
        return false;
    }
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

bool JobsList::isTimeout(pid_t pid) const
{
    for (std::vector<JobEntry>::const_iterator it = (this->jobs_vector).begin(); it != jobs_vector.end(); ++it){
        if (it->m_pid == pid  && it->m_isTimeout) {
            return true;
        }
    }
    return false;
}

bool JobsList::killTimeoutBecauseOfAlarm()
{
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies(); // TODO: Check this!
    std::time_t now = std::time(nullptr);

    // std::cout << "got alarm and now need to kill first job in timeout_vector to have a life > its timeout. Jobs: \n";
    // this->printJobsList();

    for (std::vector<JobEntry>::const_iterator it = (this->jobs_vector).begin(); it != jobs_vector.end(); ++it){
        // std::cout << "checking job: " << it->m_cmd_line << " has difftime: " << std::to_string(std::difftime(now, it->m_init)) << std::endl;
        if (std::difftime(now, it->m_init)  >= it->m_timeout_duration) {
            // If elapsed time is larger than duration then kill the process
            std::cout << "smash: "<< it->m_cmd_line << " timed out!\n";
            kill(it->m_pid, SIGKILL);
            return true;
        }
    }
    return false;
}

void JobsList::sortVectorByJobId()
{
   sort(jobs_vector.begin(), jobs_vector.end(), [](const JobEntry& lhs, const JobEntry& rhs) {
      return lhs.m_job_id < rhs.m_job_id; // COPIED FROM INTERNET, but i think it's fine just couldn't remember 'sort()'
   });
}
// --------------------------------
// ------- EXTERNAL COMMAND -------
// --------------------------------

ExternalCommand::ExternalCommand(const char* cmd_line, bool is_alarm, int timeout_duration, const char* full_timeout_str) 
: Command(cmd_line), is_alarm(is_alarm), timeout_duration(timeout_duration),  full_timeout_str(full_timeout_str){
    _updateCommandForExternalComplex(cmd_line_clean, cmd_args_clean_external);
}

void ExternalCommand::execute()
{
    int stat;
    if (this->args_count_clean < 1) {
        // std::cout << " Too few words\n"; // TODO: Erase, figure out what should actually be printed
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
            if (!this->is_alarm) {
                smash.addJob(pid, cmd_line, 0, false, this->is_alarm, this->timeout_duration);
            } else {
                smash.addJob(pid, this->full_timeout_str, 0, false,  this->is_alarm, this->timeout_duration);
                smash.addTimeoutToList(pid, this->full_timeout_str, this->timeout_duration);
            }
        } 

        // FOREGROUND
        else {
            // Update Foreground PID to be that of Child
            smash.updateFgPid(pid);
            smash.updateFgJobId(0);
            if (!this->is_alarm) {
                smash.updateFgCmdLine(this->cmd_line); 
            } else {
                smash.updateFgCmdLine(this->full_timeout_str);
                smash.addTimeoutToList(pid, this->full_timeout_str, this->timeout_duration); // I can assume if timeout the command is real
            }
                      
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
        smash.updateFgJobId(0);
        // If in background, then don't wait!
    }
}

// ----------------------------------
// ------- SPECIAL COMMANDS ---------
// -----------------------------------

// PIPES

PipeCommand::PipeCommand(const char* cmd_line) : Command(cmd_line) {}

void PipeCommand::execute()
{
    // CHECK input
    // if (args_count_clean != 3) {
    //     std::cerr << "smash error: pipe: invalid arguments\n";
    //     return;
    // }

    // OUT CHANNEL
    int outChannel;
    if (_isErrPipe(cmd_line_clean)) {
        outChannel = 2;
    } else {
        outChannel = 1;
    }

    // INPUT
    char leftPart[81];
    char rightPart[81];

    if (!_splitSidesIntoTwoCharArrays(leftPart, rightPart, cmd_line_clean)) {
        std::cerr << "smash error: redirection: invalid arguments\n";
        return;      
    }
    SmallShell& smash = SmallShell::getInstance();

    // PIPE
    int fd[2];
    pipe(fd);

    if (fork() == 0) {
        // first child 
        dup2(fd[1],outChannel);
        close(fd[0]);
        close(fd[1]);
        smash.executeCommand(leftPart);
        // Close child
        smash.setKillSmash();
        return;
    }
    if (fork() == 0) { 
        // second child 
        dup2(fd[0],0);
        close(fd[0]);
        close(fd[1]);
        smash.executeCommand(rightPart);
        // Close child
        smash.setKillSmash();
        return;
    }
    close(fd[0]);
    close(fd[1]);

    int status;
    wait(&status);
    wait(&status);
}

// REDIRECTION COMMAND

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute()
{
    // if (args_count_clean < 3) {
    //     std::cerr << "smash error: pipe: invalid arguments\n";
    //     return;
    // }
    bool isComplex = _isComplexRedirection(cmd_line_clean);
    char leftPart[81];
    char rightPart[81];

    if (!_splitSidesIntoTwoCharArrays(leftPart, rightPart, cmd_line_clean)) {
        std::cerr << "smash error: redirection: invalid arguments\n";
        return;      
    }
    // std::cout << "left: " << leftPart << std::endl;
    // std::cout << "rightPart: " << rightPart << std::endl;


    SmallShell& smash = SmallShell::getInstance();
    // FORK
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
    }
    // "Parent > Child" || "Parent >> Child"
    int stateOpen;
    if (pid == 0) {
        // CHILD 
        close(1);
        if (!isComplex) {
            // "Parent > Child"
            stateOpen = open(rightPart, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR); // redirects output to 
            if (stateOpen == -1) {
                perror("smash error: open failed");
            }
        } 
        else {
            // "Parent >> Child"
            if (access(rightPart, F_OK) == 0) {
                // file exists
                open(rightPart, O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
            } else {
                // file doesn't exist
                open(rightPart, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR); // redirects output to file
            }
        }
        if (stateOpen != -1) {
            smash.executeCommand(leftPart);
        }
        // Close child
        smash.setKillSmash();
    } else {
        // PARENT
        wait(NULL);
    }
}


// CHMOD COMMAND

ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ChmodCommand::execute()
{
    // CHECK valid inputs
    if (args_count_clean != 3) {
        std::cerr << "smash error: setcore: invalid arguments\n";
        return;
    }
    int new_mod;
    if (!_isCharArrANumber(cmd_args_clean[1], new_mod)) {
        std::cerr << "smash error: gettype: invalid arguments\n";
        return; 
    }
    // CHMOD
    int res = chmod(cmd_args_clean[2], new_mod);
    if (res == -1) {
        perror("smash error: chmod failed");
    } 
}

// SET_CORE COMMAND

SetcoreCommand::SetcoreCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), p_jobList(jobs) {}

void SetcoreCommand::execute()
{
    // TODO: Check this function, sysconf() forwards because this material is new to me
    // CHECK valid inputs
    if (args_count_clean != 3) {
        std::cerr << "smash error: setcore: invalid arguments\n";
        return;
    }
    int coreNum, job_id;
    if (!_isCharArrAPosNumber(cmd_args_clean[2], coreNum) || !_isCharArrANumber(cmd_args_clean[1], job_id)) {
        std::cerr << "smash error: setcore: invalid arguments\n";
        return; 
    }
    pid_t job_pid;
    std::string job_cmd;
    int found_job_id = 0;
    if (!(p_jobList->jobExists(job_id, job_cmd, job_pid, false, false, found_job_id))) {
        std::cerr << "smash error: setcore: job-id "<< std::to_string(job_id) <<" does not exist\n";
        return;
    }
    //CHECK valid core
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (coreNum < 0 || coreNum >= num_cores) {
        std::cerr << "smash error: setcore: invalid core number" << std::endl;
        return;
    }
    // CHANGE CORE
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(coreNum, &mask);
    int ret = sched_setaffinity(job_pid, sizeof(mask), &mask);
    if (ret == -1) {
        perror("smash error: sched_setaffinity failed");
        return;
    }
}

// GET_FILE_TYPE COMMAND

GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetFileTypeCommand::execute()
{
    if (args_count_clean != 2) {
        std::cerr << "smash error: gettype: invalid arguments\n"; // TODO: They spelled arguments without a "g", asked in Piazza
        // don't want to fail tests because of spelling
        return;
    }
    struct stat info;
    if (stat(cmd_args_clean[1], &info) == -1) {
        perror("smash error: stat failed");
        return;
    }
    std::string type;
    if (S_ISREG(info.st_mode)) {
        type = "regular file";
    } else if (S_ISDIR(info.st_mode)) {
        type = "directory";
    } else if (S_ISLNK(info.st_mode)) {
        type = "symbolic link";
    } else if (S_ISBLK(info.st_mode)) {
        type = "block device";
    } else if (S_ISCHR(info.st_mode)) {
        type = "character device";
    } else if (S_ISFIFO(info.st_mode)) {
        type = "FIFO";
    } else if (S_ISSOCK(info.st_mode)) {
        type = "socket";
    } else {
        std::cerr << "smash error: gettype: invalid arguments\n";
        return;
    }
    long long typeSize = (long long) info.st_size;
    std::cout << cmd_args_clean[1] <<"’s type is \""<< type <<"\" and takes up "<< std::to_string(typeSize) <<" bytes\n";

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
    int found_job_id = 0;
    bool exists = false;
    exists = p_jobList->jobExists(jobId, job_cmd, job_pid, false, false, found_job_id);
    if (!exists) {
        std::cerr << "smash error: kill: job-id "<< std::to_string(jobId) <<" does not exist\n";
        return;
    }
    // std::cout << "My arguments: signal: "<< std::to_string(signal) << " , jobId: " << std::to_string(jobId) << std::endl;
    
    // Send signal to Job
    int result = kill(job_pid, signal);
    if(result == -1) {
        // perror("smash error: kill"); // TODO!!! - Check maybe need to print this instead
        std::cerr << "smash error: kill: invalid arguments\n";
    } else {
        // UPDATE JOB LIST (SIGSTOP/SIGCONT)
        std::cout << "signal number "<< std::to_string(signal) << " was sent to pid "<< std::to_string(job_pid) <<"\n";
        p_jobList->setJobPidStopState(job_pid, signal);
    }
    // Clean vectors
    SmallShell& smash = SmallShell::getInstance();
    smash.killAllZombies();
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
    int found_job_id = 0;
    bool exists = false;
    if (args_count_clean != 1) {
        // "fg <int>"
        exists = p_jobList->jobExists(std::stoi(cmd_args_clean[1]), job_cmd, job_pid, false, true, found_job_id);
    } else {
        // "fg"
        exists = p_jobList->jobExists(-1, job_cmd, job_pid, true, true, found_job_id);
    }
    if (!exists) return;

    // WAIT
    std::cout << job_cmd << " : " << std::to_string(job_pid) << std::endl;
    SmallShell& smash = SmallShell::getInstance();
    int stat;
    // Update Foreground PID to be that of Child
    smash.updateFgPid(job_pid);
    smash.updateFgCmdLine(job_cmd.c_str());
    smash.updateFgJobId(found_job_id);
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
    smash.updateFgJobId(0);
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
    int found_job_id = 0;
    bool exists = false;
    if (args_count_clean != 1) {
        // "bg <int>"
        exists = p_jobList->stoppedJobExists(std::stoi(cmd_args_clean[1]), job_cmd, job_pid, false, found_job_id);
    } else {
        // "bg"
        exists = p_jobList->stoppedJobExists(-1, job_cmd, job_pid, true, found_job_id);
    }
    if (!exists) return;

    // PRINT command
    std::cout << job_cmd << " : " << std::to_string(job_pid) << std::endl;

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
            std::perror("smash error: chdir failed"); 
            return;
        }
    } else {
        // Normal: cd <path>
        if (chdir(secondWord.c_str()) != 0) {
            // Not successful
            std::perror("smash error: chdir failed"); 
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


// TIME_OUT COMMAND

TimeoutCommand::TimeoutCommand(const char* cmd_line) : BuiltInCommand(cmd_line) , m_duration(0), m_command("")
{
    _parseTimeoutCommand(cmd_line, m_command ,m_duration); // we can assume correct input: Piazza: @57
}

void TimeoutCommand::execute()
{
    // std::cout << "command: " << m_command << std::endl;

    // TODO: Implement this
    // In JobsList::killTimeoutBecauseOfAlarm, need to make killAllZombies work for it, porbably make it a smash command
    // and not a jobsList command is best, also because you don't want the killAllZombies from timeoutCommand to ruin that for jobsList

    alarm(m_duration); // will know who to kill based on who is first in the Smash::timeoutList
    ExternalCommand* cmd = new ExternalCommand((this->m_command).c_str(), true, this->m_duration, this->cmd_line);
    if (cmd!=nullptr) {
        try {
            cmd->execute();
            delete cmd;
        } catch (const InvalidCommand & e) {
            delete cmd;
            throw InvalidCommand();
        } 
    }
}

// --------------------------
// ------- SMALL SHELL -------
// --------------------------

SmallShell::SmallShell() {
    this->smashPrompt = "smash> ";
    this->lastWorkingDirectory = ""; // uninitialized
    this->jobList = new JobsList();
    this->timeoutList = new JobsList();
}

SmallShell::~SmallShell() {
    if (this->jobList!=nullptr) delete jobList;
    if (this->timeoutList!=nullptr) delete timeoutList;
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

int SmallShell::returnFgJobId() const
{
    return fg_job_id;
}


void SmallShell::updateFgPid(const pid_t newFgPid) 
{
    this->fg_pid = newFgPid;
}

void SmallShell::updateFgCmdLine(const char * newFgCmdLine)
{
    this->fg_cmd_line = newFgCmdLine;
}

void SmallShell::updateFgJobId(const int newJobId)
{
    this->fg_job_id = newJobId;
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

void SmallShell::addJob(pid_t pid, const std::string cmd_line, int fg_jobId, bool isStopped, bool isTimeout, int duration)
{
    if (jobList == nullptr) { 
        // std::cout << "You accidentaly erased JobList!\n";
        return;
    }
    jobList->addJob(pid, cmd_line, fg_jobId, isStopped,  isTimeout, duration);
}

void SmallShell::addTimeoutToList(pid_t pid, std::string cmd_line, int duration)
{
    if (timeoutList == nullptr) { 
        // std::cout << "You accidentaly erased timeoutList!\n";
        return;
    }
    timeoutList->addJob(pid, cmd_line, 0, false,  true, duration);
}

bool SmallShell::giveAlarm()
{
    if (timeoutList == nullptr) { 
        // std::cout << "You accidentaly erased timeoutList!\n";
        return false;
    }
    return timeoutList->killTimeoutBecauseOfAlarm();
}

bool SmallShell::getKillSmash()
{
    return this->killSmash;
}

void SmallShell::setKillSmash()
{
    this->killSmash = true;
}

bool SmallShell::pidIsTimeout(pid_t pid)
{
    return this->jobList->isTimeout(pid);
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
    if (strchr(cmd_line, '>')) {
        // Is Redirection Command
        return new RedirectionCommand(cmd_line);
    } else if (strchr(cmd_line, '|')) {
        return new PipeCommand(cmd_line);
    } else if (firstWord_clean == "pwd") {
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
        return new JobsCommand(cmd_line, this->jobList); 
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
    else if (firstWord_clean == "getfiletype") {
        return new GetFileTypeCommand(cmd_line);
    }
    else if (firstWord_clean == "setcore") {
        return new SetcoreCommand(cmd_line, this->jobList);
    }
    else if (firstWord_clean == "chmod") {
        return new ChmodCommand(cmd_line);
    }
    else if (firstWord_clean == "timeout") {
        return new TimeoutCommand(cmd_line);
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
    Command* cmd = CreateCommand(cmd_line);
    if (cmd!=nullptr) {
        try {
            cmd->execute();
            delete cmd;
        } catch (const InvalidCommand & e) {
            delete cmd;
            throw InvalidCommand();
        } 
    }
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


