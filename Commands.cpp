#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
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
    for (int i=0; i < str.length(); i++) {
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
// -------------------------------
// ------- COMMAND CLASSES -------
// -------------------------------

// TODO: Add your implementation for classes in Commands.h

Command::Command(const std::string cmd_line) : cmd_str(cmd_line), word_count(0) {
    word_count = _fillVectorWithStrings(cmd_line, this->cmd_args);

    // Create Array
    int count = 0;
    for (std::vector<std::string>::const_iterator it = (this->cmd_args).begin(); it != (this->cmd_args).end() && count < 80; ++it) {
        cmd_args_array[count] = new char[(*it).size() + 1];
        strcpy(cmd_args_array[count], (*it).c_str()); // TODO: SERIOUSLY check that I'm not leaking memory everywhere
        count++;
    }

    // Set last to be NULL
    cmd_args_array[count] = NULL;
}

Command::~Command() {
    for (int i = 0; i < (this->cmd_args).size(); ++i) {
        delete[] cmd_args_array[i];
    }
}

// --------------------------------
// ------- EXTERNAL COMMAND -------
// --------------------------------

ExternalCommand::ExternalCommand(const std::string cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute()
{
    // TODO: The following is a basic implementation for commands like "date", "ls", "ls -a" that run in foreground, do the rest
    if (this->word_count < 1) {
        std::cout << " Too few words\n"; // TODO: Erase, figure out what should actually be printed (although should never get here)
        return;
    }

    // UPDATE COMMAND
    _updateCommandForExternal(this->cmd_args_array); // Alternatively, use execvp() instead of execv() !!!!! // TODO: use execvp() instead?

    // FORK
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
    }

    // CHILD
    else if (pid == 0) {
        setpgrp();
        execv(cmd_args_array[0], cmd_args_array); // works for example if you enter "/bin/date" into bash
        std::cerr << "Error executing command\n";
        // EXIT ?
        exit(EXIT_FAILURE);
    }

    // PARENT
    else {
        wait(NULL); // TODO: Implement status
        // If in background, then don't wait
    }
}

// ----------------------------------
// ------- BUILT IN COMMANDS -------
// -----------------------------------

BuiltInCommand::BuiltInCommand(const std::string cmd_line) : Command(cmd_line) {}

// CHANGE_DIR COMMAND

ChangeDirCommand::ChangeDirCommand(const std::string cmd_line, std::string* lastPwd) : BuiltInCommand(cmd_line), lastPwd(lastPwd) {}

void ChangeDirCommand::execute()
{
    // Get current directory
    std::string currDir = "/";
    char cwd[OUTPUT_MAX_OUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currDir = cwd;
    }

    // See if there's a third word (illegal)
    if (this->word_count > 2) {
        std::cerr << "smash error: cd: too many arguments\n";
        return;
    }
    // See if there's only one word->"cd" (illegal?) // TODO: what to do if only get "cd" (or "cd&")?
    std::string secondWord = _findXthWord(this->cmd_args, 1);

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

GetCurrDirCommand::GetCurrDirCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) {}

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

ShowPidCommand::ShowPidCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) {}

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

SmallShell::~SmallShell() {
// TODO: add your implementation
}

std::string SmallShell::getSmashPrompt() const
{
    return this->smashPrompt;
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
Command * SmallShell::CreateCommand(const std::string cmd_line) {

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
        return new ChangeDirCommand(cmd_s_clean, this->getLastWorkingDirectoryPointer());
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

void SmallShell::executeCommand(const std::string cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  if (cmd!=nullptr) {
      cmd->execute();
  }
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}