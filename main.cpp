#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <fcntl.h>
using namespace std;
vector<string> parseArguments(const string &command)
{
    vector<string> arguments;
    string arg;

    bool insideSingleQuotes = false;
    bool insideDoubleQuotes = false;

    for (char c : command)
    {
        if (c == '\'' && !insideDoubleQuotes)
        {
            insideSingleQuotes = !insideSingleQuotes;
        }
        else if (c == '"' && !insideSingleQuotes)
        {
            insideDoubleQuotes = !insideDoubleQuotes;
        }
        else if (isspace(c) && !insideSingleQuotes && !insideDoubleQuotes)
        {
            if (!arg.empty())
            {
                arguments.push_back(arg);
                arg.clear();
            }
        }
        else
        {
            arg += c;
        }
    }

    if (!arg.empty())
    {
        arguments.push_back(arg);
    }

    return arguments;
}
int executeSimpleCommand(const string &command)
{
    vector<string> arguments = parseArguments(command);

    if (arguments.empty())
    {
        return 0;
    }

    vector<char *> args;

    for (string &argument : arguments)
    {
        args.push_back(const_cast<char *>(argument.c_str()));
    }

    args.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(args[0], args.data());

        cout << "Command not found!" << endl;
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }

        return 1;
    }
    else
    {
        cout << "Fork failed!" << endl;
        return 1;
    }
}
int main()
{
    // cout << "Hello from a process!" << endl;
    string command;
    cout << "Welcome to AashShell!" << endl;
    while (true)
    {

        cout << "aash$ " << flush;
        if (!getline(cin, command))
        {
            cout << endl;
            break;
        }
        if (command == "exit")
        {
            cout << "Exiting AashShell..." << endl;
            cout << "GoodBye!" << endl;
            break;
        }
        size_t redirectPosition = command.find('>');
        size_t inputRedirectPosition = command.find('<');
        bool appendMode = false;

        if (redirectPosition != string::npos &&
            redirectPosition + 1 < command.length() &&
            command[redirectPosition + 1] == '>')
        {
            appendMode = true;
        }
        if (appendMode &&
            redirectPosition + 2 < command.length() &&
            command[redirectPosition + 2] == '>')
        {
            cout << "AashShell: invalid redirection" << endl;
            continue;
        }
        if (inputRedirectPosition != string::npos)
        {
            string commandPart = command.substr(0, inputRedirectPosition);
            string filePart = command.substr(inputRedirectPosition + 1);

            // Remove leading/trailing spaces from filename
            size_t start = filePart.find_first_not_of(' ');
            size_t end = filePart.find_last_not_of(' ');

            if (start != string::npos)
            {
                filePart = filePart.substr(start, end - start + 1);
            }

            int fileDescriptor = open(filePart.c_str(), O_RDONLY);
            pid_t pid = fork();

            if (pid == 0)
            {
                dup2(fileDescriptor, STDIN_FILENO);

                close(fileDescriptor);

                vector<string> arguments = parseArguments(commandPart);

                if (arguments.empty())
                {
                    exit(0);
                }

                vector<char *> args;

                for (string &argument : arguments)
                {
                    args.push_back(const_cast<char *>(argument.c_str()));
                }

                args.push_back(nullptr);

                execvp(args[0], args.data());

                cout << "Command not found!" << endl;
                exit(1);
            }
            else if (pid > 0)
            {
                close(fileDescriptor);
                waitpid(pid, NULL, 0);
            }
            else
            {
                close(fileDescriptor);
                cout << "Fork failed!" << endl;
            }

            continue;
        }
        size_t orPosition = command.find("||");

        if (orPosition != string::npos)
        {
            string firstCommand = command.substr(0, orPosition);
            string secondCommand = command.substr(orPosition + 2);

            int status = executeSimpleCommand(firstCommand);

            if (status != 0)
            {
                executeSimpleCommand(secondCommand);
            }

            continue;
        }

        size_t pipePosition = command.find('|');
        if (redirectPosition != string::npos)
        {
            string commandPart = command.substr(0, redirectPosition);
            string filePart;

            if (appendMode)
            {
                filePart = command.substr(redirectPosition + 2);
            }
            else
            {
                filePart = command.substr(redirectPosition + 1);
            }

            // Remove leading/trailing spaces from filename
            size_t start = filePart.find_first_not_of(' ');
            size_t end = filePart.find_last_not_of(' ');

            if (start != string::npos)
            {
                filePart = filePart.substr(start, end - start + 1);
            }

            int flags = O_WRONLY | O_CREAT;

            if (appendMode)
            {
                flags |= O_APPEND;
            }
            else
            {
                flags |= O_TRUNC;
            }

            int fileDescriptor = open(filePart.c_str(), flags, 0644);

            if (fileDescriptor == -1)
            {
                cout << "Failed to open file!" << endl;
                continue;
            }

            pid_t pid = fork();

            if (pid == 0)
            {
                dup2(fileDescriptor, STDOUT_FILENO);

                close(fileDescriptor);

                vector<string> arguments = parseArguments(commandPart);

                if (arguments.empty())
                {
                    exit(0);
                }

                vector<char *> args;

                for (string &argument : arguments)
                {
                    args.push_back(const_cast<char *>(argument.c_str()));
                }

                args.push_back(nullptr);

                execvp(args[0], args.data());

                cout << "Command not found!" << endl;
                exit(1);
            }
            else if (pid > 0)
            {
                close(fileDescriptor);
                waitpid(pid, NULL, 0);
            }
            else
            {
                close(fileDescriptor);
                cout << "Fork failed!" << endl;
            }

            continue;
            if (fileDescriptor == -1)
            {
                cout << "Failed to open file!" << endl;
                continue;
            }
        }

        if (pipePosition != string::npos)
        {
            string firstCommand = command.substr(0, pipePosition);
            string secondCommand = command.substr(pipePosition + 1);

            int pipefd[2];

            if (pipe(pipefd) == -1)
            {
                cout << "Pipe creation failed!" << endl;
                continue;
            }

            pid_t firstPid = fork();

            if (firstPid == 0)
            {
                dup2(pipefd[1], STDOUT_FILENO);

                close(pipefd[0]);
                close(pipefd[1]);

                vector<string> arguments = parseArguments(firstCommand);

                vector<char *> args;

                for (string &argument : arguments)
                {
                    args.push_back(const_cast<char *>(argument.c_str()));
                }

                args.push_back(nullptr);

                execvp(args[0], args.data());

                cout << "Command not found!" << endl;
                exit(1);
            }

            pid_t secondPid = fork();

            if (secondPid == 0)
            {
                dup2(pipefd[0], STDIN_FILENO);

                close(pipefd[0]);
                close(pipefd[1]);

                vector<string> arguments = parseArguments(secondCommand);

                vector<char *> args;

                for (string &argument : arguments)
                {
                    args.push_back(const_cast<char *>(argument.c_str()));
                }

                args.push_back(nullptr);

                execvp(args[0], args.data());

                cout << "Command not found!" << endl;
                exit(1);
            }

            close(pipefd[0]);
            close(pipefd[1]);

            waitpid(firstPid, NULL, 0);
            waitpid(secondPid, NULL, 0);

            continue;
        }
        if (command == "pwd")
        {
            char cwd[1024];

            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                cout << cwd << endl;
            }
            else
            {
                cout << "pwd: error getting current directory" << endl;
            }

            continue;
        }
        if (command == "cd" || command.rfind("cd ", 0) == 0)
        {
            string path;

            if (command == "cd")
            {
                path = getenv("HOME");
            }
            else
            {
                path = command.substr(3);
            }

            if (path[0] == '~')
            {
                path = string(getenv("HOME")) + path.substr(1);
            }

            if (chdir(path.c_str()) != 0)
            {
                cout << "cd: directory not found" << endl;
            }

            continue;
        }
        size_t andPosition = command.find("&&");

        if (andPosition != string::npos)
        {
            string firstCommand = command.substr(0, andPosition);
            string secondCommand = command.substr(andPosition + 2);

            int status = executeSimpleCommand(firstCommand);

            if (status == 0)
            {
                executeSimpleCommand(secondCommand);
            }

            continue;
        }
        if (command.rfind("echo ", 0) == 0)
        {
            string text = command.substr(5);
            string result;

            bool insideSingleQuotes = false;
            bool insideDoubleQuotes = false;

            for (size_t i = 0; i < text.length(); i++)
            {
                if (text[i] == '\'' && !insideDoubleQuotes)
                {
                    insideSingleQuotes = !insideSingleQuotes;
                    continue;
                }

                if (text[i] == '"' && !insideSingleQuotes)
                {
                    insideDoubleQuotes = !insideDoubleQuotes;
                    continue;
                }

                if (text[i] == '$' && !insideSingleQuotes)
                {
                    string variable;
                    i++;

                    while (i < text.length() &&
                           (isalnum(text[i]) || text[i] == '_'))
                    {
                        variable += text[i];
                        i++;
                    }

                    i--;

                    const char *value = getenv(variable.c_str());

                    if (value != nullptr)
                    {
                        result += value;
                    }
                }
                else
                {
                    result += text[i];
                }
            }

            cout << result << endl;
            continue;
        }
        if (command.rfind("export ", 0) == 0)
        {
            string assignment = command.substr(7);

            size_t equalsPosition = assignment.find('=');

            if (equalsPosition == string::npos)
            {
                cout << "export: invalid syntax" << endl;
                continue;
            }

            string variable = assignment.substr(0, equalsPosition);
            string value = assignment.substr(equalsPosition + 1);

            if (variable.empty() ||
                !(isalpha(variable[0]) || variable[0] == '_'))
            {
                cout << "export: invalid variable name" << endl;
                continue;
            }

            bool validVariable = true;

            for (size_t i = 1; i < variable.length(); i++)
            {
                if (!(isalnum(variable[i]) || variable[i] == '_'))
                {
                    validVariable = false;
                    break;
                }
            }

            if (!validVariable)
            {
                cout << "export: invalid variable name" << endl;
                continue;
            }

            setenv(variable.c_str(), value.c_str(), 1);

            continue;
        }
        if (command.rfind("unset ", 0) == 0)
        {
            string variable = command.substr(6);

            if (variable.empty())
            {
                cout << "unset: missing variable name" << endl;
                continue;
            }

            bool validVariable = true;

            if (!(isalpha(variable[0]) || variable[0] == '_'))
            {
                validVariable = false;
            }

            for (size_t i = 1; i < variable.length(); i++)
            {
                if (!(isalnum(variable[i]) || variable[i] == '_'))
                {
                    validVariable = false;
                    break;
                }
            }

            if (!validVariable)
            {
                cout << "unset: invalid variable name" << endl;
                continue;
            }

            unsetenv(variable.c_str());

            continue;
        }
        if (command == "help")
        {
            cout << "AashShell commands:" << endl;
            cout << "  cd       Change directory" << endl;
            cout << "  pwd      Print working directory" << endl;
            cout << "  echo     Print text" << endl;
            cout << "  export   Set environment variable" << endl;
            cout << "  unset    Remove environment variable" << endl;
            cout << "  exit     Exit AashShell" << endl;
            cout << "  help     Show this help message" << endl;
            continue;
        }
        executeSimpleCommand(command);
    }
    return 0;
}