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
    struct sockaddr_in client_addr;
} SOCKET;

typedef struct TRIAL {
    string guess;
    int nB;
    int nW;
    int time;
} TRIAL;

class Server {
    string gsip, gsport;
    int verbose;
    int topscore_requests;
    SOCKET UDPsocket;
    SOCKET TCPsocket;

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
        int getScore(int n_tries);
        void end_game(string plid, int status);
        vector<TRIAL> GetTrials(FILE *file);
        int VerifyOngoing(string plid);
        int create_dir(const char* dirname);
        int FindGame(string PLID, char *fname);
        int FindLastGame(char *PLID, char *fname);
        int FindTopScores(string& message);
};

#endif