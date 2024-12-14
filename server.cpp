#include "server.hpp"
#include "constants.hpp"
#include "utils.hpp"


Server::Server(int argc, char** argv) {
    verbose = 0;

    connection_input(argc, argv);
    setup_UDP(gsport);
    setup_TCP(gsport);
    receive_request();
}


void Server::connection_input(int argc, char** argv) {
    char opt;
    extern char* optarg;

    while((opt = getopt(argc, argv, "p:v:")) != -1) {
        switch(opt) {
            case 'p':
                gsport = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p GSport] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (gsport.empty()) {
        gsport = GSPORT_DEFAULT;
    }
}

void Server::setup_UDP(string port) {

    int fd;
    struct addrinfo hints, *res;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int errcode = getaddrinfo(NULL, gsport.c_str(), &hints, &res);
    if (errcode == -1) {
        fprintf(stderr, "Unable to get address info.\n");
        exit(EXIT_FAILURE);
    }

    int n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "Unable to bind.\n");
        exit(EXIT_FAILURE);
    }

    UDPsocket.fd = fd;

}

void Server::setup_TCP(string port) {
    
    int fd;
    struct addrinfo hints, *res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int errcode = getaddrinfo(NULL, gsport.c_str(), &hints, &res);
    if (errcode == -1) {
        fprintf(stderr, "Unable to get address info.\n");
        exit(EXIT_FAILURE);
    }

    int n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "Unable to bind.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 10) == -1) {
        fprintf(stderr, "Unable to listen.\n");
        exit(EXIT_FAILURE);
    }

    TCPsocket.fd = fd;

}

void Server::receive_request(){

    // int tcp_fd, new_tcp_fd;
    // struct addrinfo hints, *res;
    // socklen_t addrlen;
    // struct sockaddr_in addr;
    // char bufferTCP[4];

    // tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if (tcp_fd == -1) {
    //     fprintf(stderr, "Unable to create socket.\n");
    //     exit(EXIT_FAILURE);
    // }

    // bzero(&hints, sizeof(hints));
    // hints.ai_family = AF_INET;
    // hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;

    // int errcode = getaddrinfo(NULL, gsport.c_str(), &hints, &res);
    // if (errcode == -1) {
    //     fprintf(stderr, "Unable to get address info.\n");
    //     exit(EXIT_FAILURE);
    // }

    // int n = bind(tcp_fd, res->ai_addr, res->ai_addrlen);
    // if (n == -1) {
    //     fprintf(stderr, "Unable to bind.\n");
    //     exit(EXIT_FAILURE);
    // }

    // if (listen(tcp_fd, 10) == -1) {
    //     fprintf(stderr, "Unable to listen.\n");
    //     exit(EXIT_FAILURE);
    // }

    int udp_fd, tcp_fd, new_fd, n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char bufferUDP[128], bufferTCP[4];
    fd_set ready_set, current_set;

    udp_fd = UDPsocket.fd;
    tcp_fd = TCPsocket.fd;

    int max_fd = std::max(udp_fd, tcp_fd);

    while (true) {
        FD_ZERO(&ready_set);
        FD_SET(udp_fd, &ready_set);
        FD_SET(tcp_fd, &ready_set);
        write(1, "AAA", 4);

        if (select(max_fd + 1, &ready_set, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "Unable to select.\n");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(tcp_fd, &ready_set)) {
            addrlen = sizeof(addr);
            if ((new_fd = accept(tcp_fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
                fprintf(stderr, "Unable to accept.\n");
                exit(EXIT_FAILURE);
            }

            n = receiveTCP(new_fd, bufferTCP, 4);
            write(1, bufferTCP, n);
            close(new_fd);
        }

        if (FD_ISSET(udp_fd, &ready_set)) {
            addrlen = sizeof(addr);
            if ((n = recvfrom(udp_fd, bufferUDP, 128, 0, (struct sockaddr*)&addr, &addrlen)) < 0) {
                fprintf(stderr, "Unable to receive message through UDP.\n");
                exit(EXIT_FAILURE);
            }
            write(1, bufferUDP, n);
            string jj = "jjboce\n";
            if (sendto(udp_fd, jj.c_str(), sizeof(jj), 0, (struct sockaddr*)&addr, addrlen) < 0) {
                fprintf(stderr, "Unable to send message through UDP.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // while(1) {
    //     addrlen = sizeof(addr);
    //     if ((new_fd = accept(tcp_fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
    //         fprintf(stderr, "Unable to accept.\n");
    //         exit(EXIT_FAILURE);
    //     }

    //     n = receiveTCP(new_fd, bufferTCP, 4);

    //     write(1, bufferTCP, n);

    //     close(new_fd);
    // }

    // int udp_fd;
    // struct addrinfo hints, *res;
    // socklen_t addrlen;
    // struct sockaddr_in addr;
    // char bufferUDP[128];

    // udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (udp_fd == -1) {
    //     fprintf(stderr, "Unable to create socket.\n");
    //     exit(EXIT_FAILURE);
    // }

    // bzero(&hints, sizeof(hints));
    // hints.ai_family = AF_INET;
    // hints.ai_socktype = SOCK_DGRAM;
    // hints.ai_flags = AI_PASSIVE;

    // int errcode = getaddrinfo(NULL, gsport.c_str(), &hints, &res);
    // if (errcode == -1) {
    //     fprintf(stderr, "Unable to get address info.\n");
    //     exit(EXIT_FAILURE);
    // }

    // int n = bind(udp_fd, res->ai_addr, res->ai_addrlen);
    // if (n == -1) {
    //     fprintf(stderr, "Unable to bind.\n");
    //     exit(EXIT_FAILURE);
    // }

    // while(1) {
    //     addrlen = sizeof(addr);

    //     n = recvfrom(udp_fd, bufferUDP, 128, 0, (struct sockaddr*)&addr, &addrlen);

    //     write(1, bufferUDP, n);
    // }

}

int Server::create_dir(char * dirname) {
    if (mkdir(dirname, 0700) == -1){
        fprintf(stderr, "Unable to create %s directory.\n", dirname);
        return -1;
    }
    return 1;
}


void Server::handle_error() {

}

int main(int argc, char** argv) {

    Server(argc, argv);

    return 0;
}