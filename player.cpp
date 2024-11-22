#include "player.hpp"
#include "constants.hpp"


Player::Player(int argc, char** argv) {
    plid = "";
    max_playtime = 0;

    connection_input(argc, argv);

    connect_UDP(gsip, gsport);
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
    struct addrinfo hints,*res;
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

//TODO: not sure if we need to save the socket;
//     freeaddrinfo(res);
//     close(fd);
}

void Player::command_input() {
    char str[MAX_LINE_SIZE];
    vector<string> line; 

    while(1) {
        while (getline(std::cin, str, ' ')) {
            line.push_back(str);
        }

        const char* command = line[0].c_str();
        if (!strcmp(command, START)) {
            if (line[1].length() != 6) {}
            
            start_cmd();
        }

        else if (!strcmp(command, TRY)) {
            try_cmd();
        }

        else if (!strcmp(command, SHOW_TRIAL) || !strcmp(command, ST)) {
            show_trials_cmd();
        }

        else if (!strcmp(command, SCOREBOARD) || !strcmp(command, SB)) {
            score_board_cmd();
        }
    }
}


void Player::start_cmd() {
    char line[MAX_LINE_SIZE];

    if (!fgets(line, sizeof(line), stdin)) {
        exit(EXIT_FAILURE);
    }

}


int main(int argc, char** argv) {

    Player(argc, argv);

    return 0;
}