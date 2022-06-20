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
#include <fstream>
#include <libgen.h>
#include <dirent.h>

#define STACK 8192
#define PROCESS_ID 1
#define RELEASE_MODE 1
#define MODE_MKDIR 0755
#define ERROR_MSG "system error: "
#define CGROUP_PROCS_PATH "/sys/fs/cgroup/pids/cgroup.procs"
#define PID_PATH "/sys/fs/cgroup/pids/pids.max"
#define RELEASE_RESOURCES_PATH "/sys/fs/cgroup/pids/notify_on_release"
#define PROC_PATH "proc"
#define REMOVE_COMMAND "rm -rf "
#define DELETE_ALL_FROM_SYS "/sys/*"
#define INDEX_OF_PATH_FILE_NAME 4
using namespace std;
struct ArgsForChild {
    char *new_hostname;
    char *new_filesystem_directory;
    char *num_processes;
    char *path_to_program;
    char **args_for_program;
    char *stack;
};


void printError(const string &msg, const char *pointer = nullptr) {
    cerr << ERROR_MSG << msg << endl;
    if (pointer){
        free((void *) pointer);
    }
    exit(1);
}


void concatenate(const char* str1,const char* str2, char** result){
    unsigned int lengthOfProcPath = strlen(str1) + strlen(str2) + 1;
    *result = (char*) malloc(sizeof (char)* lengthOfProcPath);
    strcpy(*result, str1);
    strcat(*result, str2);

}
int child(void *args) { //TODO change format for args

    ArgsForChild *argsForChild = (ArgsForChild *) args;
    char *hostname = argsForChild->new_hostname;
    char *file_directory = argsForChild->new_filesystem_directory;

    // change hostname
    if (sethostname(hostname, strlen(hostname)) == -1) {
        printError("problem with hostname",argsForChild->stack);
    }

    // change root directory
    if (chroot(file_directory) == -1) {
        printError("problem with changing root directory",argsForChild->stack);
    }

    //move to the new root directory
    if (chdir("/") == -1) {
        printError("problem changing the current working directory",argsForChild->stack);
    }

    // attach the container process into this new cgroup
    try {
        ofstream file;
        file.open(CGROUP_PROCS_PATH);
        file << PROCESS_ID;
        chmod(CGROUP_PROCS_PATH,MODE_MKDIR);
        file.close();
    }
    catch (const exception &e) {
        printError("problem with proc file access",argsForChild->stack);
    }

    // limit the number of processes:
    if(mkdir("/sys/fs/",MODE_MKDIR)|
    mkdir("/sys/fs/cgroup/", MODE_MKDIR)|
    mkdir("/sys/fs/cgroup/pids", MODE_MKDIR) == -1){
        printError("problem with creating dirs",argsForChild->stack);
    }



    //set max process number
    try {
        ofstream file;
        file.open(PID_PATH);
        file << argsForChild->num_processes;
        chmod(PID_PATH,MODE_MKDIR);
        file.close();

    }
    catch (const exception &e) {
        printError("problem with accessing pid file",argsForChild->stack);
    }


    //release resources of container
    try {
        ofstream file;
        file.open(RELEASE_RESOURCES_PATH);
        file << RELEASE_MODE;
        chmod(RELEASE_RESOURCES_PATH,MODE_MKDIR);
        file.close();
    }
    catch (const exception &e) {
        printError("problem creating file notify_on_release",argsForChild->stack);
    }

    //mount
    if (mount("proc", "/proc", "proc", 0, 0) == -1) {
        printError("problem with mounting",argsForChild->stack);
    }


    //run the new program
    if (execvp(argsForChild->path_to_program, argsForChild->args_for_program) == -1) {
        printError("problem running new program",argsForChild->stack);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char *stack = (char *) malloc(STACK);
    char *topOfStack = stack + STACK;
    if (!stack) {
        printError("problem with memory allocation");
    }

    struct ArgsForChild argsForChild{argv[1],
                                     argv[2],
                                     argv[3],
                                     argv[4]
    };
    char *fileSystemWithSlash;
    concatenate(argsForChild.new_filesystem_directory,"/",&fileSystemWithSlash);

    argsForChild.new_filesystem_directory = fileSystemWithSlash;
    argsForChild.args_for_program = argv + INDEX_OF_PATH_FILE_NAME;
    argsForChild.stack = stack;
    // create new process - will be used as a container
    int child_pid = clone(child,
                          topOfStack,
                          CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD,
                          &argsForChild);
    if (child_pid == -1) {
        free(stack);
        printError("error with creating clone");
    }
    wait(nullptr);


    //concatenate
    char *procPath;
    concatenate(fileSystemWithSlash,PROC_PATH,&procPath);


    //unmount
    if (umount(procPath) != 0) {
        free(stack);
        free(procPath);
        printError("problem with Unmounting");
    }


    //delete files created for container

    char *cmd_command = new char[strlen(argsForChild.new_filesystem_directory)+20]; //TODO change number
    strcpy(cmd_command,REMOVE_COMMAND);
    strcat(cmd_command, argsForChild.new_filesystem_directory);
    strcat(cmd_command,DELETE_ALL_FROM_SYS);
    try {
        system(cmd_command);
    }
    catch (const exception &e) {
        free(stack);
        free(procPath);
        printError("problem with deleting files");
    }


    //deallocs
    free(stack);
    free(procPath);

}


