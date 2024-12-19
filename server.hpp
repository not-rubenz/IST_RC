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
#include <fcntl.h>
#include <dirent.h>
#include <map>
#include <iomanip>

using std::string;
using std::vector;

typedef struct SOCKET {
    int fd;
    struct addrinfo *res;
    struct sockaddr_in addr;
} SOCKET;

typedef struct GAME {
    string plid;
    string mode;
    string colors;
    int max_playtime;
    time_t start_time;
    int n_tries;
    vector<string> tries;
    int score;
    int on_going;
} GAME;

class Server {
    string gsip, gsport;
    int verbose;
    SOCKET UDPsocket;
    SOCKET TCPsocket;
    std::map<string, GAME> games;

    public:
        Server(int argc, char** argv);

    private:
        void connection_input(int argc, char** argv);
        void setup_UDP(string port);
        void setup_TCP(string port);
        void setup_server(string port);
        void receive_request();
        string handle_request_udp(char* requestBuffer);
        string handle_request_tcp(int fd, char* requestBuffer);
        string handle_error(int errcode);
        string start_game(vector<string> request);
        string try_colors(vector<string> request);
        string debug_mode(vector<string> request);
        string quit_game(vector<string> request);
        string show_trials(string plid);
        string scoreboard();
        void getScore(string plid);
        void end_game(string plid, int status);
        int create_dir(const char* dirname);
        int FindLastGame(char *PLID, char *fname);
        int FindTopScores(string& message);
};

#endif