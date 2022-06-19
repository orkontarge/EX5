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
#include <libgen.h>

#define STACK 8192
#define PROCESS_ID 1
#define RELEASE_MODE 1
#define MODE_MKDIR 0755
#define ERROR_MSG "system error: "
#define PATH_OF_PIDS "/sys/fs/cgroup/pids"
#define CGROUP_PROCS_PATH "/sys/fs/cgroup/pids/cgroup.procs" //TODO: relative or absolute path
#define PID_PATH "/sys/fs/cgroup/pids/pids.max" //TODO same debate - relative or absolute path.
#define RELEASE_RESOURCES_PATH "/sys/fs/cgroup/pids/notify_on_release" //TODO where is this located??
#define PROC_PATH "proc"

#define INDEX_OF_PATH_FILE_NAME 4
using namespace std;

struct ArgsForChild {
    char *new_hostname;
    char *new_filesystem_directory;
    char *num_processes;
    char *path_to_program;
    char **args_for_program; //TODO: need to check if ** is fine. Before it was *agrs[]

};


void printError(const string &msg) {
    cerr << ERROR_MSG << msg << endl;
    //TODO: deallocte everything
    exit(1);
}

int mkpath(const char *dir, mode_t mode) {
    struct stat sb{};

    if (!dir) {
        errno = EINVAL;
        return 1;
    }

    if (!stat(dir, &sb))
        return 0;

    mkpath(dirname(strdupa(dir)), mode);

    return mkdir(dir, mode);
}


int child(void *args) { //TODO change format for args

    ArgsForChild *argsForChild = (ArgsForChild *) args;
    char *hostname = argsForChild->new_hostname;
    char *file_directory = argsForChild->new_filesystem_directory;

    // change hostname
    if (sethostname(hostname, strlen(hostname)) == -1) {
        printError("problem with hostname");
    }

    // change root directory
    if (chroot(file_directory) == -1) {
        printError("problem with changing root directory");
    }

    //move to the new root directory //TODO: check if this is the right order
    if (chdir("/") == -1) {
        printError("problem changing the current working directory");
    }

    // limit the number of processes:
    if (mkpath(PATH_OF_PIDS, MODE_MKDIR) != 0) {
        printError("problem with creating new directory");
    }

    // attach the container process into this new cgroup
    try {
        ofstream file;
        file.open(CGROUP_PROCS_PATH);
        file << PROCESS_ID;
        file.close();
    }
    catch (const exception &e) {
        printError("problem with proc file access");
    }

    //set max process number
    try {
        ofstream file;
        file.open(PID_PATH);
        file << argsForChild->num_processes;
        file.close();

    }
    catch (const exception &e) {
        printError("problem with accessing pid file");
    }


    //release resources of container
    try {
        ofstream file;
        file.open(RELEASE_RESOURCES_PATH);
        file << RELEASE_MODE;
        file.close();
    }
    catch (const exception &e) {
        printError("problem releasing resource of container");
    }

    //mount
    if (mount("proc", "/proc", "proc", 0, 0) == -1) {
        printError("problem with mounting");
    }

    //run the new program
    if (execvp(argsForChild->path_to_program, argsForChild->args_for_program) == -1) {
        printError("problem running new program");
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

    argsForChild.args_for_program = argv + INDEX_OF_PATH_FILE_NAME;

    // create new process - will be used as a container
    int child_pid = clone(child,
                          topOfStack,
                          CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD,
                          &argsForChild);
    if (child_pid == -1) {
        printError("error with creating clone");
    }
    wait(NULL);


    //TODO not sure about the order of the rest of the code. release, umount then delete?

    //concatenate
    unsigned int lengthOfProcPath = strlen(argsForChild.new_filesystem_directory) + strlen(PROC_PATH) + 1;
    char *procPath = (char*) malloc(sizeof (char)* lengthOfProcPath);
    strcpy(procPath, argsForChild.new_filesystem_directory);
    strcat(procPath, PROC_PATH);

    //unmount
    if (umount(procPath) == -1) { //TODO check if this is the right path
        printError("problem with Unmounting");
    }

    //deallocs
    free(stack);
    free(procPath);

//    //delete files created for container
//    if (unlink(PID_PATH) == -1) { //TODO check if this is the right path
//        printError("problem with unlinking");
//    }
    //TODO files to delete: files - pid.max,cgroup.procs, directories - fs,pids,cgroups
}

