#ifndef __H_SERVER
#define __H_SERVER

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

using std::string;
using std::vector;


typedef struct SOCKET {
    int fd;
    struct addrinfo *res;
    struct sockaddr_in addr;
} SOCKET;

class Server {
    string gsip, gsport;
    int verbose;
    SOCKET UDPsocket;
    SOCKET TCPsocket;

    public:
        Server(int argc, char** argv);

    private:
        void connection_input(int argc, char** argv);
        void setup_UDP(string port);
        void setup_TCP(string port);
        void receive_request();
        void handle_error();
        int create_dir(char* dirname);
};

#endif