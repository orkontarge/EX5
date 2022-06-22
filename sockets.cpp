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
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#define MAX_CLIENTS 5
#define CLIENT 4
#define SERVER 3
#define EXIT_FAILURE 1
#define BUFFER_SIZE 256
#define MAXHOSTNAME 256
#define ERROR_MSG "system error: "
using namespace std;

//TODO cannot connect to server. client throws error? look at forum.

void printError(const string &msg) {
    cerr << ERROR_MSG << msg << endl;
    exit(1);
}


//create welcome socket for server: socket -> bind -> listen
int establish(unsigned short portnum) {
    char myname[MAXHOSTNAME+1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;

    //hostnet initialization
    gethostname(myname, MAXHOSTNAME);
    hp = gethostbyname(myname);
    if (hp == nullptr){
        printError("problem with server hostname");
        exit(EXIT_FAILURE);
    }

    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    //host address
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    //port number
    sa.sin_port= htons(portnum);

    //create socket
    if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printError("problem with creating server socket");
        exit(EXIT_FAILURE);
    }

    //bind socket: give address to listen on
    if (bind(s , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0) {
        close(s);
        printError("problem with binding socket");
        exit(EXIT_FAILURE);
    }

    if(listen(s, MAX_CLIENTS)==-1){
        printError("problem with listening");
        exit(EXIT_FAILURE);
    }
    return(s);
}


// connection socket for server: accept
int get_connection(int s) {
    int t; /* socket of connection */

    if ((t = accept(s,NULL,NULL)) < 0) {
        printError("problem with accept");
        exit(EXIT_FAILURE);
    }
    return t;

}

//TODO who is this hostname??
//create client socket: socket -> connect
int call_socket(char *hostname, unsigned short portnum) {

    struct sockaddr_in sa;
    struct hostent *hp;
    int s;

    if ((hp= gethostbyname (hostname)) == nullptr) {
        printError("problem with getting hostname in client");
        exit(EXIT_FAILURE);
    }

    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr ,hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);
    if ((s = socket(hp->h_addrtype,
                    SOCK_STREAM,0)) < 0) {
        printError("problem with creating client socket");
        exit(EXIT_FAILURE);
    }

    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        close(s);
        printError("problem with connecting");
        exit(EXIT_FAILURE);
    }
    return(s);
}

int read_data(int s, char *buf, int n) {
    int bcount;       /* counts bytes read */
    int br;               /* bytes read this pass */
    bcount= 0; br= 0;

    while (bcount < n) { /* loop until full buffer */
        br = read(s, buf, n-bcount);
        if (br > 0)  {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            printError("problem with reading");
            exit(EXIT_FAILURE);
        }
    }
    return(bcount);
}

int write_data(int s, char *buf, int n) {
    int bcount;       /* counts bytes read */
    int br;               /* bytes read this pass */
    bcount= 0; br= 0;

    while (bcount < n) { /* loop until full buffer */
        br = write(s, buf, n-bcount);
        if (br > 0)  {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            printError("problem with writing");
            exit(EXIT_FAILURE);        }
    }
    return(bcount);
}

int main(int argc, char *argv[]) {
    if(argc==SERVER){
        int welcome_socket;
        int connected_socket;
        welcome_socket = establish(stoi(argv[2]));
        while(true){ //server waits for client connection
            char buf[BUFFER_SIZE];
            //TODO the connected socket connects to any client that calls it? no need to give him the calling client?
            connected_socket = get_connection(welcome_socket);

            read_data(connected_socket,buf,BUFFER_SIZE);
            if(system(buf) == -1){
                printError("problem with running terminal command");
                exit(EXIT_FAILURE);
            }
        }


    }
    if(argc>=CLIENT){
        char hostname[MAXHOSTNAME + 1];
        gethostname(hostname,MAXHOSTNAME);
        int client_socket;

        //concat terminal_command
        char terminal_command[256];
        for (int i = 3; i<argc ;i++){
            strcat(terminal_command,argv[i]);
            strcat(terminal_command," ");

        }

        client_socket = call_socket(hostname,stoi(argv[2]));
        //TODO read reply to confirm connection - how?

        write_data(client_socket,terminal_command,BUFFER_SIZE);

    }
}