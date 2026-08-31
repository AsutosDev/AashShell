#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <fcntl.h>
#include <cstdio>
#include <termios.h>
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
void executeArguments(vector<string> &arguments)
{
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
int executePipeline(const string &command)
{
    vector<string> commands;
    size_t start = 0;

    while (true)
    {
        size_t position = command.find('|', start);

        if (position == string::npos)
        {
            commands.push_back(command.substr(start));
            break;
        }

        commands.push_back(command.substr(start, position - start));
        start = position + 1;
    }

    vector<pid_t> pids;

    int previousRead = -1;

    for (size_t i = 0; i < commands.size(); i++)
    {
        int pipefd[2];

        if (i < commands.size() - 1)
        {
            if (pipe(pipefd) == -1)
            {
                cout << "Pipe creation failed!" << endl;
                return 1;
            }
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            if (previousRead != -1)
            {
                dup2(previousRead, STDIN_FILENO);
                close(previousRead);
            }

            if (i < commands.size() - 1)
            {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }

            vector<string> arguments = parseArguments(commands[i]);

            executeArguments(arguments);
        }

        if (pid > 0)
        {
            pids.push_back(pid);

            if (previousRead != -1)
            {
                close(previousRead);
            }

            if (i < commands.size() - 1)
            {
                close(pipefd[1]);
                previousRead = pipefd[0];
            }
        }
        else
        {
            cout << "Fork failed!" << endl;
            return 1;
        }
    }

    if (previousRead != -1)
    {
        close(previousRead);
    }

    for (pid_t pid : pids)
    {
        waitpid(pid, NULL, 0);
    }

    return 0;
}
size_t findOperator(const string &command, const string &op)
{
    bool insideSingleQuotes = false;
    bool insideDoubleQuotes = false;

    for (size_t i = 0; i < command.length(); i++)
    {
        char c = command[i];

        if (c == '\'' && !insideDoubleQuotes)
        {
            insideSingleQuotes = !insideSingleQuotes;
        }
        else if (c == '"' && !insideSingleQuotes)
        {
            insideDoubleQuotes = !insideDoubleQuotes;
        }

        if (!insideSingleQuotes && !insideDoubleQuotes)
        {
            if (command.substr(i, op.length()) == op)
            {
                return i;
            }
        }
    }

    return string::npos;
}
string readCommand(vector<string> &history)
{
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    string command;
    size_t cursorPosition = 0;
    int historyPosition = history.size();

    while (true)
    {
        char c;

        if (read(STDIN_FILENO, &c, 1) <= 0)
        {
            break;
        }

        if (c == '\n')
        {
            cout << endl;
            break;
        }

        // Arrow keys begin with ESC [
        if (c == 27)
        {
            char sequence[2];

            if (read(STDIN_FILENO, &sequence[0], 1) <= 0)
                continue;

            if (read(STDIN_FILENO, &sequence[1], 1) <= 0)
                continue;

            if (sequence[0] == '[')
            {
                if (sequence[1] == 'A') // Up arrow
                {
                    if (!history.empty() && historyPosition > 0)
                    {
                        historyPosition--;
                        command = history[historyPosition];
                        cursorPosition = command.length();

                        cout << "\r\033[K";
                        cout << "aash$ " << command;
                        cout.flush();
                    }
                }
                else if (sequence[1] == 'B') // Down arrow
                {
                    if (historyPosition < (int)history.size() - 1)
                    {
                        historyPosition++;
                        command = history[historyPosition];
                        cursorPosition = command.length();

                        cout << "\r\033[K";
                        cout << "aash$ " << command;
                        cout.flush();
                    }
                    else
                    {
                        historyPosition = history.size();
                        command.clear();
                        cursorPosition = 0;

                        cout << "\r\033[K";
                        cout << "aash$ ";
                        cout.flush();
                    }
                }
                else if (sequence[1] == 'D') // Left arrow
                {
                    if (cursorPosition > 0)
                    {
                        cursorPosition--;
                        cout << "\033[D";
                        cout.flush();
                    }
                }
                else if (sequence[1] == 'C') // Right arrow
                {
                    if (cursorPosition < command.length())
                    {
                        cursorPosition++;
                        cout << "\033[C";
                        cout.flush();
                    }
                }
            }
        }
        else if (c == 127)
        {
            if (cursorPosition > 0)
            {
                command.erase(cursorPosition - 1, 1);
                cursorPosition--;

                cout << "\r\033[K";
                cout << "aash$ " << command;

                for (size_t i = cursorPosition; i < command.length(); i++)
                {
                    cout << '\b';
                }

                cout.flush();
            }
        }
        else
        {
            command.insert(cursorPosition, 1, c);
            cursorPosition++;

            cout << "\r\033[K";
            cout << "aash$ " << command;

            for (size_t i = cursorPosition; i < command.length(); i++)
            {
                cout << '\b';
            }

            cout.flush();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return command;
}
int main()
{
    vector<string> history;
    int historyPosition = -1;
    // cout << "Hello from a process!" << endl;
    string command;
    cout << "Welcome to AashShell!" << endl;
    while (true)
    {

        cout << "aash$ " << flush;
        command = readCommand(history);
        if (command == "exit")
        {
            cout << "Exiting AashShell..." << endl;
            cout << "GoodBye!" << endl;
            break;
        }
        if (!command.empty())
        {
            history.push_back(command);
        }
        if (findOperator(command, "&&") != string::npos)
        {
            size_t pos = findOperator(command, "&&");

            string left = command.substr(0, pos);
            string right = command.substr(pos + 2);

            if (left.find_first_not_of(' ') == string::npos ||
                right.find_first_not_of(' ') == string::npos)
            {
                cout << "AashShell: invalid operator usage" << endl;
                continue;
            }
        }

        if (findOperator(command, "||") != string::npos)
        {
            size_t pos = findOperator(command, "||");

            string left = command.substr(0, pos);
            string right = command.substr(pos + 2);

            if (left.find_first_not_of(' ') == string::npos ||
                right.find_first_not_of(' ') == string::npos)
            {
                cout << "AashShell: invalid operator usage" << endl;
                continue;
            }
        }
        size_t redirectPosition = findOperator(command, ">");
        size_t inputRedirectPosition = findOperator(command, "<");
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
        size_t orPosition = findOperator(command, "||");

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
        bool invalidPipe = false;

        for (size_t i = 0; i < command.length(); i++)
        {
            if (command[i] != '|')
            {
                continue;
            }

            // Ignore || because it is a logical operator
            if (i + 1 < command.length() && command[i + 1] == '|')
            {
                i++;
                continue;
            }

            // Single pipe cannot be first or last
            if (i == 0 || i == command.length() - 1)
            {
                invalidPipe = true;
                break;
            }

            // Single pipe cannot be followed by another pipe
            if (i + 1 < command.length() && command[i + 1] == '|')
            {
                invalidPipe = true;
                break;
            }

            // Single pipe cannot be preceded by another pipe
            if (i > 0 && command[i - 1] == '|')
            {
                invalidPipe = true;
                break;
            }

            // Check whether there is actually a command after the pipe
            size_t next = i + 1;
            while (next < command.length() && isspace(command[next]))
            {
                next++;
            }

            if (next == command.length() || command[next] == '|')
            {
                invalidPipe = true;
                break;
            }

            // Check whether there is actually a command before the pipe
            size_t previous = i;
            while (previous > 0 && isspace(command[previous - 1]))
            {
                previous--;
            }

            if (previous == 0)
            {
                invalidPipe = true;
                break;
            }
        }

        if (invalidPipe)
        {
            cout << "AashShell: invalid pipe usage" << endl;
            continue;
        }
        size_t pipePosition = findOperator(command, "|");
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
            executePipeline(command);
            continue;
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
        size_t andPosition = findOperator(command, "&&");

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