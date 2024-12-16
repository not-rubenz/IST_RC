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
#include <vector>
#include <fcntl.h>

using std::string;
using std::vector;

typedef struct SOCKET {
    int fd;
    struct addrinfo *res;
} SOCKET;

class Player {
    string gsip, gsport;
    string plid;
    string max_playtime;
    int n_tries;
    SOCKET UDPsocket;
    SOCKET TCPsocket;

    public:
        Player(int argc, char** argv);

    private:
        void connection_input(int argc, char** argv);
        void connect_UDP(string ip, string port);
        void connect_TCP(string ip, string port);
        void command_input();
        void start_cmd(string line);
        void try_cmd(string line);
        void show_trials_cmd();
        void score_board_cmd();
        void quit_cmd();
        void exit_cmd();
        void player_terminate();
        void debug_cmd(string line);
        void tcp_terminate();
};

#endif 