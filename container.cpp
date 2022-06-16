#include <iostream>
#include <sched.h>
#include <cstdio>
#include <csignal>
#include <sched.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <string>

#include <cstring>

#define STACK 8192
#define ERROR_MSG "system error: %s \n"

using namespace std;

void printError(const string& msg){
    fprintf(stderr,ERROR_MSG, msg.c_str());
}

int child(void* new_hostname) {
    // set hostname
    char* name = (char *)new_hostname;
    if (sethostname(name, strlen(name) ) == -1){
        printError("problem with hostname");
    }
    return 0;
}

int main(int argc, char* argv[]) {


    void* stack = malloc(STACK); //TODO dealloc

    string new_hostname = argv[0];
    string new_filesystem_directory = argv[1];
    string num_processes = argv[2];
    string path_to_program = argv[3];
    string args_for_program = argv[4]; //TODO get all program parameters till argc


    // create new process - will be used as a container
    int child_pid = clone(child, stack+STACK, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD
            , args_for_program);
    wait(NULL);
}

