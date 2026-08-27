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
        getline(cin, command);
        if (command == "cd")
        {
            chdir(getenv("HOME"));
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

            for (size_t i = 0; i < text.length(); i++)
            {
                if (text[i] == '$')
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
            stringstream ss(command);
            vector<string> arguments;
            string arg;

            while (ss >> arg)
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