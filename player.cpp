#include "player.hpp"
#include "constants.hpp"
#include "utils.hpp"


Player::Player(int argc, char** argv) {
    plid = "600000";
    max_playtime = 400;
    tries = 1;

    connection_input(argc, argv);

    connect_UDP("tejo.tecnico.ulisboa.pt", "58011");
    // connect_UDP(gsip, gsport);
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
                fprintf(stderr, "Usage: %s [-n DSIP] [-p DSport]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (gsip.empty()) {
        gsip = GSIP_DEFAULT;
    }
    if (gsport.empty()) {
        gsport = GSPORT_DEFAULT;
    }
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [-n DSIP] [-p DSport]\n", argv[0]);
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

    UDPsocket.fd = fd;
    UDPsocket.res = res;
//TODO: not sure if we need to save the socket;
//     freeaddrinfo(res);
//     close(fd);
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
    }
}


void Player::start_cmd(string line) {
    ssize_t n;
    char buffer[128];
    string message = "SNG" +  line + '\n';
    n = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    n = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);

    write(1,"echo: ",6);
    write(1,buffer, n);

}


void Player::try_cmd(string line) {
    char buffer[128];
    ssize_t n;
    string message = "TRY " + plid + line + ' ' + std::to_string(tries) +'\n';
    n = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    n = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    write(1,"echo: ",6);
    write(1, buffer, n);
    tries++;
}

void Player::show_trials_cmd() {

}


void Player::score_board_cmd() {

}

int main(int argc, char** argv) {

    Player(argc, argv);

    return 0;
}