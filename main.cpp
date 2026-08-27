#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cctype>
using namespace std;
int main()
{
    // cout << "Hello from a process!" << endl;
    string command;
    cout << "Welcome to AashShell!" << endl;
    while (true)
    {

        cout << "aash$ ";
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
        if (command == "cd")
        {
            chdir(getenv("HOME"));
            continue;
        }
        size_t pipePosition = command.find('|');

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

                stringstream ss(firstCommand);
                vector<string> arguments;
                string arg;

                while (ss >> arg)
                {
                    arguments.push_back(arg);
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

            pid_t secondPid = fork();

            if (secondPid == 0)
            {
                dup2(pipefd[0], STDIN_FILENO);

                close(pipefd[0]);
                close(pipefd[1]);

                stringstream ss(secondCommand);
                vector<string> arguments;
                string arg;

                while (ss >> arg)
                {
                    arguments.push_back(arg);
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

            close(pipefd[0]);
            close(pipefd[1]);

            waitpid(firstPid, NULL, 0);
            waitpid(secondPid, NULL, 0);

            continue;
        }
        if (command.rfind("cd ", 0) == 0)
        {
            string path = command.substr(3);

            if (chdir(path.c_str()) != 0)
            {
                cout << "cd: directory not found" << endl;
            }

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
        pid_t pid = fork();
        if (pid == 0)
        {
            vector<string> arguments;
            string arg;
            bool insideQuotes = false;

            for (char c : command)
            {
                if (c == '"')
                {
                    insideQuotes = !insideQuotes;
                }
                else if (c == ' ' && !insideQuotes)
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
        }
        else if (pid > 0)
        {
            waitpid(pid, NULL, 0);
            // cout << "this is a parent process with id: "<<getpid()<<endl;
        }
        else
        {
            cout << "Error" << endl;
        }
    }
    return 0;
}