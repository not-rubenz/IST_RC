#include "player.hpp"
#include "constants.hpp"
#include "utils.hpp"


Player::Player(int argc, char** argv) {
    plid = "";
    max_playtime = "";
    n_tries = 1;

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
    struct addrinfo hints, *res;

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
    struct addrinfo hints, *res;

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

        else if (!strcmp(command, QUIT)) {
            quit_cmd();
        }

        else if (!strcmp(command, EXIT)) {
            exit_cmd();
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
    
    char cmd[4], status[4];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RSG") & !strcmp(status, "OK")) {
        plid = input[0];
        max_playtime = input[1];
    }
}


void Player::try_cmd(string line) {
    ssize_t ret;
    string message = "TRY " + plid + line + ' ' + std::to_string(n_tries) +'\n';
    char buffer[128];
    
    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);
    write(1, buffer, ret);

    char cmd[4], status[4];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RTR") & !strcmp(status, "OK")) {
        sscanf(buffer, "%*s %*s %d", &n_tries);
        n_tries++;
    }
}

void Player::show_trials_cmd() {
    int fd;
    ssize_t ret;
    string message = "STR " + plid + "\n";
    char buffer[2048], status[4], Fname[24], Fsize[3];

    connect_TCP(gsip, gsport);

    ret = sendTCP(TCPsocket.fd, message, strlen(message.c_str()));
    if (ret == -1) exit(-1);

    ret = receiveTCP(TCPsocket.fd, buffer, 2048);
    if (ret == -1) exit(-1);

    sendTCP(1, buffer, ret);

    if (strcmp(buffer, "ERR")) {
        sscanf(buffer, "RST %s", status);
        if (!strcmp(status, "ACT") || !strcmp(status, "FIN")) {
            sscanf(buffer, "RST %s %s %s", status, Fname, Fsize);
            string buffer_string = string(buffer);
            buffer_string.erase(0, buffer_string.find("\n") + 1);
            if ((fd = open(Fname, O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
                fprintf(stderr, "Error opening file.\n");
                exit(EXIT_FAILURE);
            }
            sendTCP(fd, buffer_string.c_str(), atoi(Fsize));
            close(fd);
        }
    }
    tcp_terminate();
}


void Player::score_board_cmd() {
    int fd;
    ssize_t ret;
    string message = "SSB\n";
    char buffer[2048], status[4], Fname[24], Fsize[4];

    connect_TCP(gsip, gsport);

    ret = sendTCP(TCPsocket.fd, message, message.size());
    if (ret == -1) exit(-1);

    ret = receiveTCP(TCPsocket.fd, buffer, 2048);
    if (ret == -1) exit(-1);

    sendTCP(1, buffer, ret);

    if (strcmp(buffer, "ERR")) {
        sscanf(buffer, "RSS %s", status);
        if (!strcmp(status, "OK")) {
            sscanf(buffer, "RSS %s %s %s", status, Fname, Fsize);
            string buffer_string = string(buffer);
            buffer_string.erase(0, buffer_string.find("\n") + 1);
            if ((fd = open(Fname, O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
                fprintf(stderr, "Error opening file.\n");
                exit(EXIT_FAILURE);
            }
            sendTCP(fd, buffer_string.c_str(), atoi(Fsize));
            close(fd);
        }
    }
    tcp_terminate();

}

void Player::quit_cmd() {
    ssize_t ret;
    string message;
    char buffer[128];
    if (!strcmp(plid.c_str(), "")) {
        message = "QUT\n";
    } else {
        message = "QUT " + plid + '\n';
    }

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) exit(-1);
    write(1, buffer, ret);
}

void Player::exit_cmd() {
    quit_cmd();
    player_terminate();
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
    freeaddrinfo(TCPsocket.res);
    close(TCPsocket.fd);
}

void Player::player_terminate() {
    freeaddrinfo(UDPsocket.res);
    close(UDPsocket.fd);
    exit(0);
}


int main(int argc, char** argv) {

    Player(argc, argv);

    return 0;
}