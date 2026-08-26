#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;
int main(){
    //cout << "Hello from a process!" << endl;
    string command;
    cout << "Welcome to AashShell!" << endl;
    while (true) {
        
        cout << "aash$ ";
        getline(cin,command);
        if (command == "exit") {
            cout << "Exiting AashShell..."<< endl;
            cout <<"GoodBye!" << endl;
            break;
        }
        pid_t pid = fork();
        if (pid==0){
        char* args[] = {const_cast<char*>(command.c_str()), nullptr};
        execvp(args[0], args);
        cout << "Command not found!" << endl;
        // cout << "this is a child process with id: " << getpid() <<endl;
    }
    else if (pid > 0){
        waitpid(pid, NULL, 0);
        // cout << "this is a parent process with id: "<<getpid()<<endl;
    }
    else{
        cout<<"Error"<<endl;
    }
    }
    return 0;
}