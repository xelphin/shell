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

std::string _findXthWord(const std::string str_full, int x)
{
    if (x<0) return "";
    std::string str = _trim(string(str_full));
    int endFirst = str.find_first_of(" \n");
    std::string firstWord = str.substr(0, endFirst);
    int countBlanks = 0;
    // First Word
    if (x == 0) {
        return firstWord;
    }
    // Word from second and beyond
    for (int i=endFirst; str[i] != '\n' && i < str.length(); i++) {
        if (str[i]==' ' && str[i+1]!=' ') {
            countBlanks++;
        }
        else if (str[i]!=' ' && countBlanks == x) {
            return _trim(str.substr(i,str.find_first_of(" \n", i+1) - i));
        }
    }
    return "";
}

// -------------------------------
// ------- COMMAND CLASSES -------
// -------------------------------

Command::Command(const std::string cmd_line) : cmd_str(cmd_line) {
    // initialize other fields if you write them
}

Command::~Command() {
    // cleanup
}

BuiltInCommand::BuiltInCommand(const std::string cmd_line) : Command(cmd_line) {
    // initialize other fields if you write them
}

// INDIVIDUAL COMMANDS

ChangeDirCommand::ChangeDirCommand(const std::string cmd_line, std::string* lastPwd) : BuiltInCommand(cmd_line), lastPwd(lastPwd) {
    // initialize other fields if you write them
}

void ChangeDirCommand::execute()
{
    // Get current directory
    std::string currDir = "/";
    char cwd[OUTPUT_MAX_OUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        currDir = cwd;
    }

    // See if there's a third word (illegal)
    if (_findXthWord(this->cmd_str, 2) != "") {
        std::cout << "smash error: cd: too many arguments\n";
        return;
    }
    // See if there's only one word->"cd" (illegal?) // TODO: what to do if only get "cd" (or "cd&")?
    std::string secondWord = _findXthWord(this->cmd_str, 1);
    if (secondWord == "") {
        std::cout << "smash error: cd: too few arguments\n";
        return;
    }
    // See if special: second word is "-"
    if (secondWord == "-") {
        if (*lastPwd == "") {
            std::cout << "smash error: cd: OLDPWD not set\n";
            return;
        }
        chdir((*lastPwd).c_str());
    } else {
        // Normal: cd <path>
        chdir(secondWord.c_str());
    }

    // Update last working directory
    *lastPwd = currDir;

    // TODO: Check if instead I have to continue going back in cd history (I think above is what they wanted)
}

GetCurrDirCommand::GetCurrDirCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) {
    // initialize other fields if you write them
}

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

ShowPidCommand::ShowPidCommand(const std::string cmd_line) : BuiltInCommand(cmd_line) {
    // initialize other fields if you write them
}

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






// TODO: Add your implementation for classes in Commands.h

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

    // Find second word and update smash prompt
    std::string secondWord = _findXthWord(clean_str, 1);
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
    // Get first word (command)
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (cmd_s == "") return nullptr; // TODO: Not sure what they want us to do in this case, but this works for now,update if needed

    // Clean (without &) version of firstWord
    char* clean_ptr = new char[cmd_s.length() + 1];
    strcpy(clean_ptr, cmd_s.c_str());
    _removeBackgroundSign(clean_ptr);
    std::string cmd_s_clean(clean_ptr);
    std::string firstWord_clean = cmd_s.substr(0, cmd_s_clean.find_first_of(" \n")); // note: chprompt& hello, shouldn't work

    // Check command
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
        return new ChangeDirCommand(cmd_line, this->getLastWorkingDirectoryPointer());
    }
    /*
    else if ...
    .....
    else {
      return new ExternalCommand(cmd_line);
    }
    */
    return nullptr;
}

void SmallShell::executeCommand(const std::string cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  if (cmd!=nullptr) {
      cmd->execute();
  }
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}