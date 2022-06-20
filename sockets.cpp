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
#define CLIENT 3
#define SERVER 2
#define EXIT_FAILURE 1
#define BUFFFER_SIZE 256
#define ERROR_MSG "system error: "
#define NULL nullptr
using namespace std;


void printError(const string &msg) {
    cerr << ERROR_MSG << msg << endl;
    exit(1);
}

//TODO THIS IS THE SERVER: socket->bind->listen
int establish(unsigned short portnum) {
    char myname[MAXHOSTNAME+1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;

    //hostnet initialization
    gethostname(myname, MAXHOSTNAME);
    hp = gethostbyname(myname);
    if (hp == NULL)
        return(-1);

    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    //host address
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    //port number
    sa.sin_port= htons(portnum);

    //create socket
    if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printError("problem with creating socket");
        exit(EXIT_FAILURE);
    }

    //bind socket to
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

//TODO THIS IS THE SERVER: after listen
int get_connection(int s) {
    int t; /* socket of connection */

    if ((t = accept(s,NULL,NULL)) < 0)
        return -1;
    return t;

}

//TODO THIS IS THE CLIENT: socket->connect
//TODO who is this hostname??
int call_socket(char *hostname, unsigned short portnum) {

    struct sockaddr_in sa;
    struct hostent *hp;
    int s;

    if ((hp= gethostbyname (hostname)) == NULL) {
        return(-1);
    }

    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr ,hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);
    if ((s = socket(hp->h_addrtype,
                    SOCK_STREAM,0)) < 0) {
        return(-1);
    }

    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        close(s);
        return(-1);
    }

    return(s);
}


int read_data(int s, char *buf, int n) {
    int bcount;       /* counts bytes read */
    int br;               /* bytes read this pass */
    bcount= 0; br= 0;

    while (bcount < n) { /* loop until full buffer */
        br = read(s, buf, n-bcount))
        if ((br > 0)  {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            return(-1);
        }
    }
    return(bcount);
}
//TODO get last function form Nmd,



int main(int argc, char *argv[]) {
    if(argc==SERVER){
        int s;
        int t;
        s = establish(*argv[1]);
        t = get_connection(s);


    }
    if(argc==CLIENT){
        char* terminal_command = argv[3];
    }
    //TODO print error number args
}