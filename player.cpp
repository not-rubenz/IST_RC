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

        else {
            string message = "Please enter a valid command.\n";
            write(1, message.c_str(), strlen(message.c_str()));
        }
    }
}


void Player::start_cmd(string line) {
    ssize_t ret;
    string message = "SNG" +  line + '\n';
    char buffer[128];

    vector<string> input = split_line(line);

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(UDPsocket.fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        fprintf(stderr, "Error sending message through UDP.\n");
        exit(EXIT_FAILURE);
    }

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        printf("Client or server couldn't receive the message, please try again.\n");
        return;
    }

    write(1, buffer, ret);
    
    string response;
    char cmd[4], status[4];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RSG")) {
        if (!strcmp(status, "OK")) {
            plid = input[0];
            max_playtime = input[1];
            n_tries = 1;
            response = "New game started (max " + max_playtime + " sec)\n";
        } else if (!strcmp(status, "NOK")) {
            response = "Error: There's an ongoing game!\n";
        } else {
            response = "Error: Invalid arguments!\n";
        }
    } else {
        response = "Error: Invalid Input.\n";
    }
    write(1, response.c_str(), strlen(response.c_str()));

}


void Player::try_cmd(string line) {
    ssize_t ret;
    string message = "TRY " + plid + line + ' ' + std::to_string(n_tries) +'\n';
    char buffer[128];
    
    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        fprintf(stderr, "Error sending message through UDP.\n");
        exit(EXIT_FAILURE);
    }

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        printf("Client or server couldn't receive the message, please try again.\n");
        return;
    }

    write(1, buffer, ret);

    /* Write response to terminal */
    string response;
    int nB, nW;
    char cmd[4], status[4], colors[8];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RTR")) {
        if (!strcmp(status, "OK")) {
            sscanf(buffer, "%*s %*s %d %d %d", &n_tries, &nB, &nW);
            n_tries++;
            response = "" "nB = " + std::to_string(nB) + ", nW = " + std::to_string(nW) + "\n";
        } else if (!strcmp(status, "DUP")) {
            response = "Error: Duplicated guess.\n";
        } else if (!strcmp(status, "INV")) {
            response = "Error: Invalid trial.\n";
        } else if (!strcmp(status, "NOK")) {
            response = "Error: Out of context.\n";
        } else if (!strcmp(status, "ENT")) {
            sscanf(buffer, "%*s %*s %[^\n]", colors);
            response = "Out of moves. The solution was: " + string(colors) + "\n";
        } else if (!strcmp(status, "ETM")) {
            sscanf(buffer, "%*s %*s %[^\n]", colors);
            response = "Out of time. The solution was: " + string(colors) + "\n";
        } else {
            response = "Error: Invalid arguments!\n";
        }
    } else {
        response = "Error: Invalid Input.\n";
    }
    write(1, response.c_str(), strlen(response.c_str()));

}

void Player::show_trials_cmd() {
    FILE *file;
    ssize_t ret;
    string message = "STR " + plid + "\n";
    char buffer[2048], Fname[24];
    int Fsize;

    connect_TCP(gsip, gsport);

    ret = sendTCP(TCPsocket.fd, message, strlen(message.c_str()));
    if (ret == -1) exit(-1);

    ret = receiveTCP(TCPsocket.fd, buffer, 2048);
    if (ret == -1) exit(-1);

    string response;
    char cmd[4], status[4];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RST")) {
        if (!strcmp(status, "NOK")) {
            response = "No games found.\n";
        } else {
            sscanf(buffer, "RST %s %s %d", status, Fname, &Fsize);
            response = string(buffer);
            response.erase(0, response.find("\n") + 1);
            if ((file = fopen(Fname, "w")) == NULL) {
                fprintf(stderr, "Error opening file.\n");
                exit(EXIT_FAILURE);
            }
            int n = fwrite(response.c_str(), sizeof(char), Fsize, file);
            if (n < Fsize) {
                fclose(file);
                fprintf(stderr, "Error writing into file.\n");
                exit(EXIT_FAILURE);
            }
            fclose(file);
        }
    } else {
        response = "Error: Invalid Input.\n";
    }
    write(1, response.c_str(), strlen(response.c_str()));
    tcp_terminate();

}


void Player::score_board_cmd() {
    FILE *file;
    ssize_t ret;
    string message = "SSB\n";
    char buffer[2048], Fname[24];
    int Fsize;

    connect_TCP(gsip, gsport);

    ret = sendTCP(TCPsocket.fd, message, message.size());
    if (ret == -1) exit(-1);

    ret = receiveTCP(TCPsocket.fd, buffer, 2048);
    if (ret == -1) exit(-1);

    string response;
    char cmd[4], status[6];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RSS")) {
        if (!strcmp(status, "OK")) {
            sscanf(buffer, "RSS OK %s %d", Fname, &Fsize);
            
            response = string(buffer);
            response.erase(0, response.find("\n") + 1);
            if ((file = fopen(Fname, "w")) == NULL) {
                fprintf(stderr, "Error opening file.\n");
                exit(EXIT_FAILURE);
            }
            int n = fwrite(response.c_str(), sizeof(char), Fsize, file);
            printf("%ld\n", strlen(response.c_str()));
            if (n < Fsize) {
                fclose(file);
                fprintf(stderr, "Error writing into file.\n");
                exit(EXIT_FAILURE);
            }
            fclose(file);
        } else {
            response = "No games found.\n";
        }
    } else {
        response = "Error: Invalid Input.\n";
    }
    write(1, response.c_str(), strlen(response.c_str()));
    tcp_terminate();
    
}

void Player::quit_cmd() {
    ssize_t ret;
    string message = "QUT " + plid + '\n';
    char buffer[128];

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        fprintf(stderr, "Error sending message through UDP.\n");
        exit(EXIT_FAILURE);
    }

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        printf("Client or server couldn't receive the message, please try again.\n");
        return;
    }

    write(1, buffer, ret);

    string response;
    char cmd[4], status[4], colors[8];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RQT")) {
        if (!strcmp(status, "OK")) {
            sscanf(buffer, "%*s %*s %[^\n]", colors);
            response = "Game quit. The solution was: " + string(colors) + "\n";
        } else if (!strcmp(status, "NOK")) {
            response = "Error: There's not an ongoing game!\n";
        } else {
            response = "Error: An error occurred!\n";
        }
    } else {
        response = "Error: Invalid Input.\n";
    }
    write(1, response.c_str(), strlen(response.c_str()));

}

void Player::exit_cmd() {
    quit_cmd();
    string response = "Exiting game...\n";
    write(1, response.c_str(), strlen(response.c_str()));
    player_terminate();
}

void Player::debug_cmd(string line) {
    ssize_t ret;
    string message = "DBG" + line + '\n';
    char buffer[128];
    vector<string> input = split_line(line);

    ret = sendto(UDPsocket.fd, message.c_str(), strlen(message.c_str()), 0, UDPsocket.res->ai_addr, UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        fprintf(stderr, "Error sending message through UDP.\n");
        exit(EXIT_FAILURE);
    }

    ret = recvfrom(UDPsocket.fd, buffer, 128, 0, UDPsocket.res->ai_addr, &UDPsocket.res->ai_addrlen);
    if (ret == -1) {
        printf("Client or server couldn't receive the message, please try again.\n");
        return;
    }

    string response;
    char cmd[4], status[4];
    sscanf(buffer, "%s %s", cmd, status);
    if (!strcmp(cmd, "RDB")) {
        if (!strcmp(status, "OK")) {
            plid = input[0];
            max_playtime = input[1];
            n_tries = 1;
            response = "New game started (max " + max_playtime + " sec)\n";
        } else if (!strcmp(status, "NOK")) {
            response = "Error: There's an ongoing game!\n";
        } else {
            response = "Error: Invalid arguments!\n";
        }
    } else {
        response = "Error: Invalid Input.\n";
    }
    write(1, response.c_str(), strlen(response.c_str()));

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