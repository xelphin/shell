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

// -------------------------------
// ------- COMMAND CLASSES -------
// -------------------------------

// TODO: Add your implementation for classes in Commands.h

Command::Command(const char* cmd_line) : cmd_line(cmd_line), args_count(0) {
    //word_count = _fillVectorWithStrings(cmd_line, this->cmd_args);
    this->args_count = _parseCommandLine(this->cmd_line, this->cmd_args);

    // Set last to be NULL
    this->cmd_args[this->args_count + 1] = NULL;
}

Command::~Command() {
    for (int i = 0; i < (this->args_count); ++i) {
        free(this->cmd_args[i]);
    }
}

// --------------------------------
// ------- EXTERNAL COMMAND -------
// --------------------------------

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {
    _updateCommandForExternalComplex(cmd_line, cmd_args_external);
}

void ExternalCommand::execute()
{
    // TODO: The following is a basic implementation for commands like "date", "ls", "ls -a" that run in foreground, do the rest
    if (this->args_count < 1) {
        std::cout << " Too few words adshfjasdfk\n"; // TODO: Erase, figure out what should actually be printed (although should never get here)
        return;
    }

    // FORK
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    }

        // CHILD
    else if (pid == 0) {
        setpgrp();
        if (_isComplex(this->cmd_line)) {
            // Is COMPLEX
            std::cout << "Is Complex\n";
            execv(cmd_args_external[0], cmd_args_external);
        } else {
            // Is SIMPLE
            std::cout << "Is Simple\n";
            execvp(this->cmd_args[0], this->cmd_args);
        }
        std::cerr << "Error executing command\n";
       throw InvalidCommand();
    }

    // PARENT
    else {
        SmallShell& smash = SmallShell::getInstance();
        smash.updateFgPid(pid); // foreground command is the child PID
        wait(NULL); // TODO: Implement status
        std::cout << "Child: " << cmd_line << ", finished\n"; // TODO: delete
        smash.updateFgPid(getpid()); // child is dead, so change back foreground to mean smash process again
        // If in background, then don't wait!
    }
}

// ----------------------------------
// ------- BUILT IN COMMANDS -------
// -----------------------------------

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}



// CHANGE_DIR COMMAND

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, std::string* lastPwd) : BuiltInCommand(cmd_line), lastPwd(lastPwd) {}

void ChangeDirCommand::execute()
{
    // Get current directory
    std::string currDir = "/";
    char cwd[OUTPUT_MAX_OUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currDir = cwd;
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
            std::perror("smash error: cd failed"); // TODO: is this how they want it?
            return;
        }
    } else {
        // Normal: cd <path>
        if (chdir(secondWord.c_str()) != 0) {
            // Not successful
            std::perror("smash error: cd failed"); // TODO: is this how they want it?
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
}

SmallShell::~SmallShell() {}

std::string SmallShell::getSmashPrompt() const
{
    return this->smashPrompt;
}


pid_t SmallShell::returnFgPid() const
{
    return fg_pid;
}

void SmallShell::updateFgPid(const pid_t newFgPid) 
{
    this->fg_pid = newFgPid;
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


