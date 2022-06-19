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
#define PATH_OF_PIDS "/sys/fs/cgroup/pids"
#define CGROUP_PROCS_PATH "/sys/fs/cgroup/pids/cgroup.procs" //TODO: relative or absolute path
#define PID_PATH "/sys/fs/cgroup/pids/pids.max" //TODO same debate - relative or absolute path.
#define RELEASE_RESOURCES_PATH "/sys/fs/cgroup/pids/notify_on_release" //TODO where is this located??
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
    char **args_for_program; //TODO: need to check if ** is fine. Before it was *agrs[]

};

class path;

int remove_directory(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;

        r = 0;
        while (!r && (p=readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2;
            buf = static_cast<char *>(malloc(len));

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);

    return r;
}
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
        chmod(CGROUP_PROCS_PATH,MODE_MKDIR);
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
        chmod(PID_PATH,MODE_MKDIR);
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
        chmod(RELEASE_RESOURCES_PATH,MODE_MKDIR);
        file.close();
    }
    catch (const exception &e) {
        printError("problem creating file notify_on_release");
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
    wait(nullptr);


    //concatenate
    char *procPath;
    concatenate(argsForChild.new_filesystem_directory,PROC_PATH,&procPath);


    //unmount
    if (umount(procPath) != 0) {
        printError("problem with Unmounting");
    }


    //delete files created for container

    char *cmd_command = new char[strlen(argsForChild.new_filesystem_directory)+20];
    strcpy(cmd_command,REMOVE_COMMAND);
    strcat(cmd_command, argsForChild.new_filesystem_directory);
    strcat(cmd_command,DELETE_ALL_FROM_SYS);
    try {
        system(cmd_command);
    }
    catch (const exception &e) {
        printError("problem with deleting files");
    }


    //deallocs
    free(stack);
    free(procPath);

}



//TODO: delete all
//char cwd[PATH_MAX];
//if (getcwd(cwd, sizeof(cwd)) != NULL) {
//printf("Current working dir: %s\n", cwd);
//} else {
//perror("getcwd() error");
//return 1;
//}