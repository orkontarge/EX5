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
#define RELEASE_MODE 1
#define MODE_MKDIR 0755
#define ERROR_MSG "system error: %s \n"
#define PATH_OF_PID "/sys/fs/cgroup/pids"
#define PROC_PATH "/sys/fs/cgroup/pids/cgroup.procs" //TODO relative or absolute path
#define PID_PATH "/sys/fs/cgroup/pids.max" //TODO same debate - relative or absolute path.
#define RELEASE_RESOURCES_PATH "/sys/fs/cgroup/pids/notify_on_release" //TODO where is this located??

using namespace std;

struct ArgsForChild{
    char* new_hostname;
    char* new_filesystem_directory;
    char* num_processes;
    char* path_to_program;
    char* args_for_program[];

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

    //move to the new root directory //TODO: check if this is the right order
    if (chdir(file_directory) == -1){
        printError("problem changing the current working directory");
    }


    // limit the number of processes:
    if (mkdir(PATH_OF_PID,MODE_MKDIR) == -1){
        printError("problem with new directory");
    }

    // attach the container process into this new cgroup
    try {
        ofstream file;
        file.open(PROC_PATH);
        file << PROCESS_ID;
        file.close();
    }
    catch(const exception &e){
        printError("problem with proc file access");
    }

    //set max process number
    try {
        ofstream file;
        file.open(PID_PATH);
        file << args.num_processes;
        file.close();
    }
    catch(const exception &e){
        printError("problem with accessing pid file");
    }

    //mount
    if (mount("proc", "/proc", "proc", 0, 0) == -1){
        printError("problem with mounting");
    }


    //run the new program
    if (execvp(args.path_to_program,args.args_for_program) == -1){
        printError("problem running new program");
    }

    return 0;
}

int main(int argc, char* argv[]) {


    void* stack = malloc(STACK); //TODO dealloc
    if (!stack){
        printError("problem with memory allocation");
    }

    int size = argc - 3;
    char* args_for_program = new char[size]; //TODO dealloc
    for (int i = 3; i < argc; ++i) {
        args_for_program[i-3] = argv[i]; //TODO problem with type
    }

    struct ArgsForChild args_for_child{argv[0],argv[1],argv[2],argv[3],args_for_program};

    // create new process - will be used as a container
    int child_pid = clone(child, stack+STACK, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD
            , args_for_child);
    wait(NULL);


    //TODO not sure about the order of the rest of the code. release, umount then delete?

    //release resources of container
    try {
        ofstream file;
        file.open(RELEASE_RESOURCES_PATH);
        file << RELEASE_MODE;
        file.close();
    }
    catch(const exception &e){
        printError("problem releasing resource of container");
    }


    //unmount
    if (umount(PROC_PATH) == -1){ //TODO check if this is the right path
        printError("problem with mounting");
    }

    //delete files created for container
    //TODO files to delete: files - pid.max,cgroup.procs, directories - fs,pids,cgroups
}

