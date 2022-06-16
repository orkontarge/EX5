#include <iostream>
#include <sched.h>
#include <cstdio>
#include <csignal>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string>
#include <cstring>
#include <sys/mount.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

#define STACK 8192
#define PROCESS_ID 1
#define MODE_MKDIR 0755
#define ERROR_MSG "system error: %s \n"
#define PATH_OF_PID "/sys/fs/cgroup/pids"
#define PROC_FILE "/sys/fs/cgroup/pids/cgroup.procs"
//TODO check file path, form pid or cgroup?

using namespace std;

struct ArgsForChild{
    char* new_hostname;
    char* new_filesystem_directory;

};
void printError(const string& msg){
    fprintf(stderr,ERROR_MSG, msg.c_str());
}

int child(const ArgsForChild& args) { //TODO change format for args


    char* hostname = args.new_hostname;
    char*  file_directory = args.new_filesystem_directory;

    // change hostname
    if (sethostname(hostname, strlen(hostname) ) == -1){
        printError("problem with hostname");
    }

    // change root directory
    if (chroot(file_directory) == -1){
        printError("problem with new root directory");
    }
    if (chdir(file_directory) == -1){
        printError("problem changing the current working directory");
    }
    //mount //TODO unmount
    mount("proc", "/proc", "proc", 0, 0);

    // limit the number of processes
    if (mkdir(PATH_OF_PID,MODE_MKDIR) == -1){
        printError("problem with new directory");
    }

    try {
        ofstream myfile;
        myfile.open(PROC_FILE);
        myfile << PROCESS_ID;
        myfile.close();
    }
    catch(const exception &e){
        printError("problem with writing to code");
    }

    return 0;
}

int main(int argc, char* argv[]) {


    void* stack = malloc(STACK); //TODO dealloc
    if (!stack){
        printError("problem with memory allocation");
    }
    string new_hostname = argv[0];
    string new_filesystem_directory = argv[1];
    string num_processes = argv[2];
    string path_to_program = argv[3];
    string args_for_program = argv[4]; //TODO get all program parameters till argc
    void* args_for_child;

    // create new process - will be used as a container
    int child_pid = clone(child, stack+STACK, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD
            , args_for_child);
    wait(NULL);
}

