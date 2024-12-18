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

            n = receiveWordTCP(new_fd, bufferTCP, 4);
            // write(1, bufferTCP, n);
            string message = handle_request_tcp(new_fd, bufferTCP);
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

            string message = handle_request_udp(bufferUDP);

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

string Server::handle_request_udp(char* requestBuffer) {
    string args;
    vector<string> request = split_line(requestBuffer);
    int request_size = request.size();
    char file_name[20];

    // if (request_size < 2) {
    //     return handle_error(INVALID_INPUT);
    // }

    if (!valid_PLID(request[1])) {
        return handle_error(INVALID_PLID);
    }

    int ret = FindGame(request[1].c_str(), file_name);

    const char* command = request[0].c_str();

    if (!strcmp(command, START_REQUEST)) {
        if (request_size != 3 || !valid_time(request[2])) {
            return handle_error(INVALID_START_ARG);
        } else if (ret) {
            time_t now = time(nullptr);
            int time_elapsed = (int) difftime(now, games[request[1]].start_time);
            if (time_elapsed >= games[request[1]].max_playtime) {
                end_game(request[1], GAME_TIMEOUT);
            } else {
                return handle_error(ONGOING_GAME);
            }
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

    // else if (!strcmp(command, SHOW_TRIAL_REQUEST)) {
    //     if (request_size != 2) {
    //         return handle_error(INVALID_ST_TAG);
    //     }
    //     if (ret) {
    //         time_t now = time(nullptr);
    //         int time_elapsed = (int) difftime(now, games[request[1]].start_time);
    //         if (time_elapsed >= games[request[1]].max_playtime) {
    //             end_game(request[1], GAME_TIMEOUT);
    //         }
    //     }
    //     return show_trials(request);
    // }

    // else if (!strcmp(command, SCOREBOARD_REQUEST)) {
    //     return scoreboard(request);
    // }

    else if (!strcmp(command, QUIT_REQUEST)) {
        if (request_size != 2) {
            return handle_error(INVALID_QUIT_ARG);
        } else if (!ret) {
            return handle_error(NO_ONGOING_GAME);
        }
        return quit_game(request);
    }

    else if (!strcmp(command, DEBUG_REQUEST)) {
        if (request_size != 7 || !valid_time(request[2])) {
            return handle_error(INVALID_DEBUG_ARG);
        }
        else if (ret) {
            time_t now = time(nullptr);
            int time_elapsed = (int) difftime(now, games[request[1]].start_time);
            if (time_elapsed >= games[request[1]].max_playtime) {
                end_game(request[1], GAME_TIMEOUT);
            } else {
                return handle_error(ONGOING_GAME);
            }
        }
        return debug_mode(request);
    }
    else {
        return handle_error(INVALID_CMD_SYNTAX);
    }

}

string Server::handle_request_tcp(int fd, char* requestBuffer) {
    char plid[7];

    if (!strcmp(requestBuffer, SHOW_TRIAL_REQUEST)) {
        int n = receiveWordTCP(fd, plid, 7);
        if (!valid_PLID(string(plid))) {
            return handle_error(INVALID_PLID);
        }

        if (FindGame(plid, NULL)) {
            time_t now = time(nullptr);
            int time_elapsed = (int) difftime(now, games[plid].start_time);
            if (time_elapsed >= games[plid].max_playtime) {
                end_game(plid, GAME_TIMEOUT);
            }
        }
        return show_trials(plid);
    }

    else if (!strcmp(requestBuffer, SCOREBOARD)) {

    }
}

string Server::handle_error(int errcode) {
    string message;
    switch (errcode) {
        case INVALID_INPUT:
        case INVALID_PLID:
        case INVALID_START_ARG:
        case INVALID_TRY_ARG:
        case INVALID_QUIT_ARG:
        case INVALID_DEBUG_ARG:
            write(1, "ERR\n", 4);
            message = "ERR\n";
            return message;
        case ONGOING_GAME:
            write(1, "RSG NOK\n", 8);
            message = "RSG NOK\n";
            return message;
        case DUPLICATED_GUESS:
            message = "RTR DUP\n";
            return message;
        case INVALID_TRIAL:
            message = "RTR INV\n";
            return message;
        case OUT_OF_CONTEXT:
            message = "RTR NOK\n";
            return message;
        case OUT_OF_GUESSES:
            message = "RTR ENT ";
            return message;
        case OUT_OF_TIME:
            message = "RTR ETM ";
            return message;
        case INVALID_COLOR:
            message = "RTR ERR\n";
            return message;
        case NO_ONGOING_GAME:
            message = "RQT NOK\n";
            return message;
        case NO_GAMES:
            message = "RST NOK\n";
            return message;
    }
    return NULL;
}

string Server::start_game(vector<string> request) {
    string PLID = request[1];
    string max_playtime = request[2];
    string player_dir = "GAMES/" + PLID;
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    GAME new_game;
    int fd;

    time_t now = time(nullptr);

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

    new_game.mode = mode;
    new_game.colors = string(code);
    new_game.max_playtime = atoi(max_playtime.c_str());
    new_game.start_time = now;
    new_game.n_tries = 0;
    new_game.score = 0;
    games[PLID] = new_game;

    string message = "RSG OK\n";
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
    GAME current_game = games[PLID];
    string target = current_game.colors;
    string guess = C1 + C2 + C3 + C4;

    time_t now = time(nullptr);
    int time_elapsed = (int) difftime(now, current_game.start_time);
    if (time_elapsed >= current_game.max_playtime) {
        end_game(PLID, GAME_TIMEOUT);
        string message = handle_error(OUT_OF_TIME) + target[0] + " " + target[1] + " " + target[2] + " " + target[3] + "\n";
        return message;
    }

    if (current_game.n_tries == MAX_TRIALS - 1) {
        end_game(PLID, GAME_FAIL);
        string message = handle_error(OUT_OF_GUESSES) + target[0] + " " + target[1] + " " + target[2] + " " + target[3] + "\n";
        return message;
    }

    if (!valid_color(request[2]) || !valid_color(request[3])
        || !valid_color(request[4]) || !valid_color(request[5])) {
            return handle_error(INVALID_COLOR);
    }
    
    if (current_game.n_tries == atoi(nT.c_str()) & current_game.n_tries != 0) {
        if (guess.compare(current_game.tries.back())) {
            return handle_error(INVALID_TRIAL);
        } else {
            games[PLID].n_tries--;
        }
    } else if (current_game.n_tries == atoi(nT.c_str()) - 1) {
        for (string s: current_game.tries) {
            if (!s.compare(guess)) {
                return handle_error(DUPLICATED_GUESS);
            }
        }
    } else {
        return handle_error(INVALID_TRIAL);
    }

    if ((fd = open(file_name.c_str(), O_WRONLY | O_APPEND, 0755)) < 0) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

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
            nW++;         // Found a correct letter in the wrong position
            target_counts[guess_index]--; // Decrement the available count
        }
    }

    sprintf(buffer, "T: %s %d %d %d\n", guess.c_str(), nB, nW, time_elapsed);
    sendTCP(fd, buffer, strlen(buffer));

    close(fd);

    games[PLID].n_tries++;
    games[PLID].tries.push_back(guess);

    if (nB == 4) {
        end_game(PLID, GAME_WIN);
    }

    string message = "RTR OK " + nT + " " + std::to_string(nB) + " " + std::to_string(nW) + " " + C1 + " " + C2 + " " + C3 + " " + C4 + "\n";
    return message;
}

string Server::debug_mode(vector<string> request) {
    string PLID = request[1];
    string max_playtime = request[2];
    string player_dir = "GAMES/" + PLID;
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    char buffer[128];
    int fd;

    time_t now = time(nullptr);
    int time_elapsed = (int) difftime(now, games[PLID].start_time);

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


string Server::show_trials(string plid) {
    string message;
    string Fdata;
    string trials = "";
    FILE *file;
    char file_name[24], buffer[128], guess[4], Fdata_aux[128], date[11], time_str[9];;
    int max_playtime, nB, nW, time_elapsed, n_trials, time_left;
    time_t start_time;
    n_trials = 0;
    if (FindGame(plid, NULL)) {
        string file_n = "GAMES/GAME_" + plid + ".txt";
        if ((file = fopen(file_n.c_str(), "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        message = "RST ACT STATE_" + plid + ".txt";
        fgets(buffer, 128, file);
        sscanf(buffer, "%*s %*s %*s %d %s %s %ld\n", &max_playtime, date, time_str, &start_time);
        while (fgets(buffer, 128, file)) {
            if (buffer[0] == 'T') {
                sscanf(buffer, "T: %s %d %d %d\n", guess, &nB, &nW, &time_elapsed);
                sprintf(Fdata_aux, "Trial: %s, nB: %d, nW: %d at %3ds\n", guess, nB, nW, time_elapsed);
                trials += string(Fdata_aux);
                n_trials++;
                // write(1, Fdata_aux, strlen(Fdata_aux));
            }
        }
        Fdata = "Active game found for player " + plid + "\n";

        time_t now = time(nullptr);
        int time_elapsed = (int) difftime(now, start_time);
        time_left = max_playtime - time_elapsed;
        sprintf(Fdata_aux, "Game initiated: %s %s with %d seconds to be completed\n\n     Game started - %d transactions found\n\n", 
                date, time_str, max_playtime, n_trials);
        
        Fdata += string(Fdata_aux) + trials +  "\n  -- " + std::to_string(time_left) +  " seconds remaining to be completed --\n\n";
        write(1, Fdata.c_str(), strlen(Fdata.c_str()));

    } else if (FindLastGame((char *) plid.c_str(), file_name)) {

    } else {
        return handle_error(NO_GAMES);
    }

}

string Server::scoreboard(vector<string> request) {
    string message;
    message = string("-------------------------------- TOP 10 SCORES --------------------------------\n\n")
            + string("           SCORE   PLAYER     CODE     NO TRIALS    MODE\n");

    
}


string Server::quit_game(vector<string> request) {
    string PLID = request[1];
    string target = games[PLID].colors;
    end_game(PLID, GAME_QUIT);

    string message = "RQT OK " + std::string(1, target[0]) + " " + std::string(1, target[1]) + " " + std::string(1, target[2]) + " " + std::string(1,target[3]) + "\n";
    return message;
}

void Server::end_game(string plid, int status) {
    int fd;
    char buffer[32];
    string s_status;
    string score_file_name;
    string file_name = "GAMES/GAME_" + plid + ".txt";
    if ((fd = open(file_name.c_str(), O_WRONLY | O_APPEND, 0755)) < 0) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }
    time_t now = time(nullptr);
    struct tm *local_time = localtime(&now);
    char date[11], time_str[9];
    int time_elapsed = std::min((int) difftime(now, games[plid].start_time), games[plid].max_playtime);
    strftime(date, sizeof(date), "%Y-%m-%d", local_time);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    sprintf(buffer, "%s %s %d\n", date, time_str, time_elapsed);
    sendTCP(fd, buffer, strlen(buffer));
    close(fd);

    char date_aux[9], time_aux[7];
    strftime(date_aux, sizeof(date_aux), "%Y%m%d", local_time);
    strftime(time_aux, sizeof(time_aux), "%H%M%S", local_time);

    switch(status) {
        case GAME_WIN:
            s_status = "_W";
            getScore(plid);
            break;
        case GAME_FAIL:
            s_status = "_F";
            games[plid].score = 0;
            break;
        case GAME_QUIT:
            s_status = "_Q";
            games[plid].score = 0;
            break;
        case GAME_TIMEOUT:
            s_status = "_T";
            games[plid].score = 0;
            break;
    }

    char score_aux[4];
    sprintf(score_aux, "%03d", games[plid].score);
    score_file_name = "SCORES/" + string(score_aux) + "_" + plid + "_" + string(date_aux) + "_" + string(time_aux) + ".txt";
    if ((fd = open(score_file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    sprintf(buffer, "%03d %s %s %d %c\n", games[plid].score, plid.c_str(), games[plid].colors.c_str(), games[plid].n_tries, games[plid].mode);
    sendTCP(fd, buffer, strlen(buffer));
    close(fd);

    string new_file_name = "GAMES/" + plid + "/" + string(date_aux) + "_" + string(time_aux) + s_status + ".txt";

    if (rename(file_name.c_str(), new_file_name.c_str()) < 0) {
        fprintf(stderr, "Error renaming file.\n");
        exit(EXIT_FAILURE);
    }

    games.erase(plid);
}

void Server::getScore(string plid) {

    games[plid].score = ((MAX_TRIALS - 1) - (games[plid].n_tries - 1)) * 100 / (MAX_TRIALS - 1);

}

// Find GAME_(PLID).txt
int Server::FindGame(std::string PLID, const char *fname) {
    DIR *dir;
    struct dirent *entry;
    char dirname[] = "GAMES/";
    std::string file_name = "GAME_" + PLID + ".txt";
    int found = 0;

    dir = opendir(dirname);
    if (dir == NULL) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, file_name.c_str())) {
            fname = file_name.c_str(); // Update fname if file is found
            found = 1;
            break;
        }
    }
    closedir(dir);
    return found;
}

int Server::FindLastGame(char* PLID, char *fname) {
    struct dirent ** filelist;
    int n_entries, found;
    char dirname[20];

    sprintf(dirname, "GAMES/%s/",PLID);
    n_entries = scandir(dirname, &filelist, 0, alphasort);
    found = 0;
    if (n_entries <= 0)
        return (0);
    else {
        while (n_entries --) {
            if(filelist[n_entries]->d_name [0]!= '.' && !found) {
                sprintf(fname, "GAMES/%s/%s",PLID, filelist[n_entries]->d_name); 
                found = 1;
            }
            free(filelist[n_entries]);
        }
        free(filelist);
    }
    return (found);
}



int main(int argc, char** argv) {

    Server(argc, argv);

    return 0;
}