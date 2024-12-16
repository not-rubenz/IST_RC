#include "server.hpp"
#include "constants.hpp"
#include "utils.hpp"


Server::Server(int argc, char** argv) {
    verbose = 0;

    connection_input(argc, argv);
    setup_server(gsport);
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

void Server::setup_server(string gsport) {

    setup_UDP(gsport);
    setup_TCP(gsport);

    if (!create_dir("GAMES")) {
        exit(EXIT_FAILURE);
    }

    if (!create_dir("SCORES")) {
        exit(EXIT_FAILURE);
    }
}

void Server::receive_request(){

    int udp_fd, tcp_fd, new_fd, n;
    socklen_t addrlen;
    char bufferUDP[128], bufferTCP[4];
    fd_set ready_set, current_set;

    udp_fd = UDPsocket.fd;
    tcp_fd = TCPsocket.fd;

    int max_fd = std::max(udp_fd, tcp_fd);

    while (true) {
        FD_ZERO(&ready_set);
        FD_SET(udp_fd, &ready_set);
        FD_SET(tcp_fd, &ready_set);

        if (select(max_fd + 1, &ready_set, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "Unable to select.\n");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(tcp_fd, &ready_set)) {
            addrlen = sizeof(TCPsocket.addr);
            if ((new_fd = accept(tcp_fd, (struct sockaddr*)&TCPsocket.addr, &addrlen)) == -1) {
                fprintf(stderr, "Unable to accept.\n");
                exit(EXIT_FAILURE);
            }

            n = receiveTCP(new_fd, bufferTCP, 4);
            write(1, bufferTCP, n);
            close(new_fd);
        }

        if (FD_ISSET(udp_fd, &ready_set)) {
            addrlen = sizeof(UDPsocket.addr);
            bzero(bufferUDP, 128);

            if ((n = recvfrom(udp_fd, bufferUDP, 128, 0, (struct sockaddr*)&UDPsocket.addr, &addrlen)) < 0) {
                fprintf(stderr, "Unable to receive message through UDP.\n");
                exit(EXIT_FAILURE);
            }
            write(1, bufferUDP, n);

            // handle_request(bufferUDP);

            // string jj = "jjboce\n";
            // if (sendto(udp_fd, jj.c_str(), sizeof(jj), 0, (struct sockaddr*)&UDPsocket.addr, addrlen) < 0) {
            //     fprintf(stderr, "Unable to send message through UDP.\n");
            //     exit(EXIT_FAILURE);
            // }

            string message = handle_request(bufferUDP);
            printf(message.c_str());

            if (sendto(udp_fd, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr*)&UDPsocket.addr, addrlen) < 0) {
                fprintf(stderr, "Unable to send message through UDP.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int Server::create_dir(const char* dirname) {
    if (mkdir(dirname, 0700) == -1){
        if (errno == EEXIST) {
            return 1;
        }
        fprintf(stderr, "Unable to create %s directory.\n", dirname);
        return -1;
    }
    return 1;
}

string Server::handle_request(char* requestBuffer) {
    string args;
    vector<string> request = split_line(requestBuffer);
    int request_size = request.size();
    char file_name[20];

    if (request_size <= 2) {
        return handle_error(INVALID_INPUT);
    }

    if (!valid_PLID(request[1])) {
        return handle_error(INVALID_PLID);
    }

    int ret = FindGame(request[1].c_str(), file_name);

    const char* command = request[0].c_str();

    if (!strcmp(command, START_REQUEST)) {
        if (request_size != 3 || !valid_time(request[2])) {
            return handle_error(INVALID_START_ARG);
        }
        else if (ret) {
            return handle_error(ONGOING_GAME);
        }
        return start_game(request);
    }

    else if (!strcmp(command, TRY_REQUEST)) {
        if (request_size != 7) {
            return handle_error(INVALID_TRY_ARG);
        } else if (!ret) {
            return handle_error(OUT_OF_CONTEXT);
        }
        return try_colors(request);
    }

    else if (!strcmp(command, SHOW_TRIAL_REQUEST)) {

    }

    else if (!strcmp(command, SCOREBOARD_REQUEST)) {

    }

    else if (!strcmp(command, QUIT_REQUEST)) {

    }

    else if (!strcmp(command, DEBUG_REQUEST)) {
        if (!strcmp(command, DEBUG_REQUEST)) {
            if (request_size != 7 || !valid_time(request[2])) {
                return handle_error(INVALID_DEBUG_ARG);
            }
            else if (ret) {
                return handle_error(ONGOING_GAME);
            }
            return debug_mode(request);
        }
    }
    else {
        return handle_error(INVALID_CMD_SYNTAX);
    }

}

string Server::handle_error(int errcode) {
    string message;
    switch (errcode) {
        case INVALID_INPUT:
        case INVALID_PLID:
        case INVALID_START_ARG:
        case INVALID_TRY_ARG:
        case INVALID_DEBUG_ARG:
            write(1, "ERR\n", 4);
            message = "ERR\n";
            return message;
        case ONGOING_GAME:
            write(1, "RSG NOK\n", 8);
            message = "RSG NOK\n";
            return message;
        case DUPLICATED_GUESS:
        case INVALID_TRIAL:
        case OUT_OF_CONTEXT:
            message = "RTR NOK\n";
            return message;
        case OUT_OF_GUESSES:
        case OUT_OF_TIME:
        case INVALID_COLOR:
            message = "RTR ERR\n";
            return message;
        default:
            break;
    }
}

string Server::start_game(vector<string> request) {
    string PLID = request[1];
    string max_playtime = request[2];
    string player_dir = "GAMES/" + PLID;
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    int fd;

    if ((fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    create_dir(player_dir.c_str());

    const char valid_colors[] = {'R', 'G', 'B', 'Y', 'O', 'P'};
    const char mode = 'P';
    char code[5];
    srand(time(nullptr));
    for (int i = 0; i < 4; ++i) {
        code[i] = valid_colors[rand() % 6]; 
    }
    code[4] = '\0';

    time_t now = time(nullptr);
    struct tm *local_time = localtime(&now);
    char date[11], time_str[9];
    strftime(date, sizeof(date), "%Y-%m-%d", local_time);   
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time); 

    dprintf(fd, "%s %c %s %s %s %s %ld\n",
            PLID.c_str(),     
            mode,             
            code,             
            max_playtime.c_str(), 
            date,           
            time_str, 
            now);


    close(fd);

    string message = "SNG OK\n";

    return message;
}

string Server::try_colors(vector<string> request) {
    string PLID = request[1];
    string C1 = request[2];
    string C2 = request[3];
    string C3 = request[4];
    string C4 = request[5];
    string nT = request[6];
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    char buffer[128];
    int fd;

    if (!valid_color(request[2]) || !valid_color(request[3])
        || !valid_color(request[4]) || !valid_color(request[5])) {
            return handle_error(INVALID_COLOR);
    }

    if ((fd = open(file_name.c_str(), O_RDWR, 0755)) < 0) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    int n = receiveTCP(fd, buffer, 128);

    char plid[7], mode[2], target[5];
    int max_time;
    time_t start_time;
    sscanf(buffer, "%s %s %s %d %*d-%*d-%*d %*d:%*d:%*d %ld", plid, 
                                                    mode, 
                                                    target,
                                                    &max_time,
                                                    &start_time);

    string guess_aux = C1 + C2 + C3 + C4;
    const char *guess = guess_aux.c_str();

    int nB = 0; // Letters in the correct position
    int nW = 0;   // Correct letters in the wrong position

    const char valid_colors[] = {'R', 'G', 'B', 'Y', 'O', 'P'};
    int target_counts[6] = {0, 0, 0, 0, 0, 0};

    // Step 1: Count correct positions and prepare counts for wrong positions
    for (size_t i = 0; i < 4; ++i) {
        if (target[i] == guess[i]) {
            nB++; // Exact match
        } else {
            int target_index = FindIndex(valid_colors, target[i], 6);
            target_counts[target_index]++; // Count available letters in target for wrong positions
        }
    }

    // Step 2: Count correct letters in wrong positions
    for (size_t i = 0; i < 4; ++i) {
        int guess_index = FindIndex(valid_colors, guess[i], 6);
        if (target[i] != guess[i] && target_counts[guess_index] > 0) {
            ++nW;         // Found a correct letter in the wrong position
            target_counts[guess_index]--; // Decrement the available count
        }
    }

    time_t now = time(nullptr);

    int time_left = (int) difftime(now, start_time);

    dprintf(fd, "T: %s %d %d %d\n", guess, nB, nW, time_left); //时间还没做

    close(fd);

    string message = "RTR OK " + nT + " " + std::to_string(nB) + " " + std::to_string(nW) + " " + C1 + " " + C2 + " " + C3 + " " + C4 + "\n";
    // string message = "RTR OK\n";
    return message;
}

string Server::debug_mode(vector<string> request) {
    string PLID = request[1];
    string max_playtime = request[2];
    string player_dir = "GAMES/" + PLID;
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    char buffer[128];
    int fd;

    if (!valid_color(request[3]) || !valid_color(request[4])
        || !valid_color(request[5]) || !valid_color(request[6])) {
            return handle_error(INVALID_COLOR);
    }

    if ((fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    create_dir(player_dir.c_str());

    const char mode = 'D';
    char code[5];
    code[0] = request[3][0];
    code[1] = request[4][0];
    code[2] = request[5][0];
    code[3] = request[6][0];
    code[4] = '\0';

    time_t now = time(nullptr);
    struct tm *local_time = localtime(&now);
    char date[11], time_str[9];
    strftime(date, sizeof(date), "%Y-%m-%d", local_time);   
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time); 

    dprintf(fd, "%s %c %s %s %s %s %ld\n",
            PLID.c_str(),     
            mode,             
            code,             
            max_playtime.c_str(), 
            date,           
            time_str, 
            now);

    close(fd);

    string message = "RDB OK\n";

    return message;
}

// Find GAME_(PLID).txt
int Server::FindGame(string PLID, const char *fname) {
    struct dirent ** filelist;
    int n_entries, found;
    char dirname [20];
    string file_name = "GAME_" + PLID + ".txt";

    sprintf(dirname, "GAMES/");
    n_entries = scandir (dirname, &filelist, 0, alphasort);
    found = 0;  

    if (n_entries <= 0) {
        return 0;
    }
    else {
        while (n_entries--) {
            if (!strcmp(filelist[n_entries]->d_name, file_name.c_str()) && !found) {
                fname = file_name.c_str();
                found = 1;
            }
            free(filelist [n_entries]);
        }
        free(filelist);
    }
    return (found);
}


int Server::FindLastGame(char* PLID, char *fname) {
    struct dirent ** filelist;
    int n_entries, found;
    char dirname [20];

    sprintf(dirname, "GAMES/%s/",PLID);
    n_entries = scandir (dirname, &filelist, 0, alphasort);
    found = 0;
    if (n_entries <= 0)
        return (0);
    else {
        while (n_entries --) {
            if(filelist [n_entries]->d_name [0]!= '.' && !found) {
                sprintf(fname, "GAMES/%s/%s",PLID, filelist [n_entries]->d_name); 
                found = 1;
            }
            free(filelist [n_entries]);
        }
        free(filelist);
    }
    return (found);
}

int main(int argc, char** argv) {

    Server(argc, argv);

    return 0;
}