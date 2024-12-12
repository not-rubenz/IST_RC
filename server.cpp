#include "server.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "utils.cpp"


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
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd == -1) {
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;// IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if(errcode != 0) {
        fprintf(stderr, "Unable to link to server.\n");
        exit(EXIT_FAILURE);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
	if(n == -1) {
        fprintf(stderr, "Unable to bind to user.\n");
        exit(EXIT_FAILURE);
    }

    //save the socket
    UDPsocket.fd = fd;
    UDPsocket.res = res;
}
 
void Server::setup_TCP(string port){
    int fd;
    int n;
    struct sockaddr_in servaddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
        fprintf(stderr, "Unable to create socket.\n");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(stoi(port));

    n = bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if(n == -1){
        fprintf(stderr, "Unable to bind.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(fd, 10) == -1){
        fprintf(stderr, "Unable to listen.\n");
        exit(EXIT_FAILURE);
    }


    TCPsocket.fd = fd;
    TCPsocket.addr = servaddr;
}

void Server::receive_request(){
    int tcpfd, connfd, udpfd, maxfdp1;
    char bufferUDP[10], bufferTCP[10];
    fd_set rset;
    ssize_t n;
    socklen_t len;
    struct sockaddr_in cliaddr;
    void sig_chld(int);
    pid_t childpid;

    tcpfd = TCPsocket.fd;
    udpfd = UDPsocket.fd;

    FD_ZERO(&rset);

    maxfdp1 = std::max(tcpfd, udpfd) + 1;
    while(1){
        FD_SET(tcpfd, &rset);
        FD_SET(udpfd, &rset);
        select(maxfdp1, &rset, NULL, NULL, NULL);
        /* TCP request */
        if (FD_ISSET(tcpfd, &rset)){
            len = sizeof(cliaddr);
            connfd = accept(tcpfd, (struct sockaddr*)&cliaddr, &len);
            if (connfd == -1){
                //TODO:handle_error();
            }
            n = receiveTCP(connfd, bufferTCP, 4);
            // if (n == -1){
            //     handle_error();
            // }    
            // handle_request(bufferTCP);
            printf("%s\n", bufferTCP);

            close(connfd);
        }
        
        // /* UDP request */
        // if (FD_ISSET(udpfd, &rset)){
        //     bzero(bufferUDP, 10);
            
        //     n = receiveUDP(socketUDP, bufferUDP);
        //     if (n == FAIL){
        //         handle_error(SERVER, SYS_CALL);
        //     }
            
        //     handle_request(bufferUDP);
        // }
    }
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