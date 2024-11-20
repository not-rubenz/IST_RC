#ifndef __H_PLAYER
#define __H_PLAYER

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#define PORT "58001"

using std::string;

class Player {
    string gsip, gsport;
    string plid;
    int max_playtime;
    // SOCKET UDPsocket;

    public:
        Player(int argc, char** argv);

    private:
        void parse_input(int argc, char** argv);
        void connect_UDP(string ip, string port);
        void handle_input(int argc, char** argv);
};

#endif 