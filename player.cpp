#include "player.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "utils.cpp"


Player::Player(int argc, char** argv) {
    plid = "";
    max_playtime = "";
    tries = 1;

    connection_input(argc, argv);

    connect_UDP(gsip, gsport);
    // connect_TCP(gsip, gsport);
    command_input();
}



void Player::connection_input(int argc, char** argv) {
    char opt;
    extern char* optarg;

    while((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch(opt) {
            case 'n':
                gsip = optarg;
                break;
            case 'p':
                gsport = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n GSIP] [-p GSport]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (gsip.empty()) {
        gsip = GSIP_DEFAULT;
    }
    if (gsport.empty()) {
        gsport = GSPORT_DEFAULT;
    }
    if (argc > 5) {
        fprintf(stderr, "Usage: %s [-n GSIP] [-p GSport]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}


void Player::connect_UDP(string ip, string port) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    fd = socket(AF_INET, SOCK_DGRAM, 0); //UDP socket
    if(fd == -1) {
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    } 

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP socket

    errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if(errcode != 0) {
        fprintf(stderr, "Unable to link to server.\n");
        exit(EXIT_FAILURE);
    } 

    // save the socket
    UDPsocket.fd = fd;
    UDPsocket.res = res;
}

void Player::connect_TCP(string ip, string port) {
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    fd = socket(AF_INET, SOCK_STREAM, 0); //TCP socket
    if(fd == -1) {
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    } 

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM; //TCP socket

    errcode = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);
    if(errcode != 0) {
        fprintf(stderr, "Unable to link to server.\n");
        exit(EXIT_FAILURE);
    } 

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1)/*error*/exit(1);

    // save the socket
    TCPsocket.fd = fd;
    TCPsocket.res = res;
}

void Player::command_input() {

    while(1) {
        string command_string;
        string args;

        std::cin >> command_string;
        std::getline(std::cin, args);

        const char* command = command_string.c_str();
        
        if (!strcmp(command, START)) {
            start_cmd(args);
        }

        else if (!strcmp(command, TRY)) {
            try_cmd(args);
        }

        else if (!strcmp(command, SHOW_TRIAL) || !strcmp(command, ST)) {
            show_trials_cmd();
        }

        else if (!strcmp(command, SCOREBOARD) || !strcmp(command, SB)) {
            score_board_cmd();
        }

        else if (!strcmp(command, DEBUG)) {
            debug_cmd(args);
        }
    }
}


void Player::start_cmd(string line) {
    ssize_t ret;
    string message = "SNG" +  line + '\n';
    char buffer[128];

    vector<string> input = split_line(line);

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    write(1, buffer, ret);
    
    if (strcmp(buffer, "ERR\n")) {
        plid = input[0];
        max_playtime = input[1];
    }
}


void Player::try_cmd(string line) {
    ssize_t ret;
    string message = "TRY " + plid + line + ' ' + std::to_string(tries) +'\n';
    char buffer[128];

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);
    write(1, buffer, ret);
    tries++;
}

void Player::show_trials_cmd() {
    ssize_t ret;
    string message = "STR " + plid + "\n";
    char buffer[2048];

    connect_TCP(gsip, gsport);

    ret = sendTCP(TCPsocket.fd, message, strlen(message.c_str()));
    if (ret == -1) exit(-1);

    ret = receiveTCP(TCPsocket.fd, buffer, 2048);
    if (ret == -1) exit(-1);

    sendTCP(1, buffer, ret);
    tcp_terminate();
}


void Player::score_board_cmd() {
    ssize_t ret;
    string message = "SSB\n";
    char buffer[2048];

    connect_TCP(gsip, gsport);

    ret = sendTCP(TCPsocket.fd, message, message.size());
    if (ret == -1) exit(-1);

    ret = receiveTCP(TCPsocket.fd, buffer, 2048);
    if (ret == -1) exit(-1);

    sendTCP(1, buffer, ret);
    tcp_terminate();

}

void Player::debug_cmd(string line) {
    ssize_t ret;
    string message = "DBG" + line + '\n';
    char buffer[128];
    vector<string> input = split_line(line);

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    write(1, buffer, ret);
    if (strcmp(buffer, "ERR\n")) {
        plid = input[0];
        max_playtime = input[1];
    }
}

void Player::tcp_terminate() {
    close(TCPsocket.fd);
}

void Player::player_terminate() {
    freeaddrinfo(UDPsocket.res);
    freeaddrinfo(TCPsocket.res);
    close(UDPsocket.fd);
    exit(0);
}


int main(int argc, char** argv) {

    Player(argc, argv);

    return 0;
}