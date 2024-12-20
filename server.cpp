#include "server.hpp"
#include "constants.hpp"
#include "utils.hpp"

/**
 * Inicializa servidor
 */
Server::Server(int argc, char** argv) {
    verbose = 0;
    topscore_requests = 0;

    connection_input(argc, argv);
    setup_server(gsport);
    receive_request();
}

/**
 * Estabeleve a conecao do servidor.
 */
void Server::connection_input(int argc, char** argv) {

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            gsport = string(argv[++i]);
            if (!isPort(gsport)) {
                fprintf(stderr, "Usage: %s [-p GSport] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        } else if (!strcmp(argv[i], "-v")) {
            verbose = 1;
        } else {
            fprintf(stderr, "Usage: %s [-p GSport] [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (gsport.empty()) {
        gsport = GSPORT_DEFAULT;
    }
    if (argc > 4) {
        fprintf(stderr, "Usage: %s [-n GSIP] [-p GSport]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}

/**
 * Inicializa o Socket UDP.
 */
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

/**
 * Inicializa o socket TCP
 */
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

/**
 * Inicializa os sockets UDP e TCP.
 * Cria as diretorias GAMES/ e SCORES/
 */
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

/**
 * Funcao que corre em loop recebendo e enviando mensagens por TCP e UDP.
 * Trata dos requests recebidos.
 */
void Server::receive_request(){

    int udp_fd, tcp_fd, new_fd, n;
    socklen_t addrlen;
    char bufferUDP[MAX_UDPBUFFER_SIZE], bufferTCP[MAX_COMMAND_SIZE];
    fd_set ready_set;

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
            addrlen = sizeof(TCPsocket.client_addr);
            if ((new_fd = accept(tcp_fd, (struct sockaddr*)&TCPsocket.client_addr, &addrlen)) == -1) {
                fprintf(stderr, "Unable to accept.\n");
                exit(EXIT_FAILURE);
            }

            n = receiveWord(new_fd, bufferTCP, 4);
            string message = handle_request_tcp(new_fd, bufferTCP);
            sendMessage(new_fd, message, message.size());
            close(new_fd);
        }

        if (FD_ISSET(udp_fd, &ready_set)) {
            addrlen = sizeof(UDPsocket.client_addr);
            bzero(bufferUDP, MAX_UDPBUFFER_SIZE);

            if ((n = recvfrom(udp_fd, bufferUDP, MAX_UDPBUFFER_SIZE, 0, (struct sockaddr*)&UDPsocket.client_addr, &addrlen)) < 0) {
                fprintf(stderr, "Unable to receive message through UDP.\n");
                exit(EXIT_FAILURE);
            }

            string message = handle_request_udp(bufferUDP);

            if (sendto(udp_fd, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr*)&UDPsocket.client_addr, addrlen) < 0) {
                fprintf(stderr, "Unable to send message through UDP.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/**
 * Funcao que comeca um jogo.
 * 
 * @param dirname nome da diretoria
 * @return Sucesso ou falha.
 */
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

/**
 * Funcao que verifica e executa comandos pedidos enviados por UDP.
 * 
 * @param requestBuffer pedido recebido
 * @return mensagem a ser enviado para o cliente
 */
string Server::handle_request_udp(char* requestBuffer) {
    string args;
    vector<string> request = split_line(requestBuffer);
    int request_size = request.size();
    string message;

    if (request_size < 2) {
        return handle_error(INVALID_INPUT);
    }

    const char* command = request[0].c_str();

    if (!strcmp(command, START_REQUEST)) {
        if (request_size != 3 || !valid_PLID(request[1]) || !valid_time(request[2])) {
            return handle_error(INVALID_START_ARG);
        }
        return start_game(request);
    }

    else if (!strcmp(command, TRY_REQUEST)) {
        if (request_size != 7 || !valid_PLID(request[1])) {
            return handle_error(INVALID_TRY_ARG);
        } else if (!FindGame(request[1], NULL)) {
            return handle_error(OUT_OF_CONTEXT);
        }
        return try_colors(request);
    }

    else if (!strcmp(command, QUIT_REQUEST)) {
        if (request_size != 2 || !valid_PLID(request[1])) {
            return handle_error(INVALID_QUIT_ARG);
        }
        return quit_game(request);
    }

    else if (!strcmp(command, DEBUG_REQUEST)) {
        if (request_size != 7 || !valid_PLID(request[1]) || !valid_time(request[2])) {
            return handle_error(INVALID_DEBUG_ARG);
        }
        return debug_mode(request);
    }
    else {
        return handle_error(INVALID_INPUT);
    }

}

/**
 * Funcao que verifica e executa comandos pedidos enviados por TCP.
 * 
 * @param requestBuffer pedido recebido
 * @return mensagem a ser enviado para o cliente
 */
string Server::handle_request_tcp(int fd, char* requestBuffer) {
    char plid[MAX_PLID_SIZE];

    if (!strcmp(requestBuffer, SHOW_TRIAL_REQUEST)) {
        receiveWord(fd, plid, 7);
        if (!valid_PLID(string(plid))) {
            return handle_error(INVALID_PLID);
        }
        return show_trials(plid);
    }

    else if (!strcmp(requestBuffer, SCOREBOARD_REQUEST)) {
        return scoreboard();
    }

    return handle_error(INVALID_INPUT);

}

/**
 * Funcao que recebe um errcode e devolve uma mensagem de erro.
 * 
 * @param errcode codigo de erro
 * @return mensagem a ser enviado para o cliente
 */
string Server::handle_error(int errcode) {
    string message;
    switch (errcode) {
        case INVALID_INPUT:
        case INVALID_PLID:
            message = "ERR\n";
            return message;
        case INVALID_START_ARG:
            message = "RSG ERR\n";
            return message;
        case INVALID_TRY_ARG:
            message = "RTR ERR\n";
            return message;
        case INVALID_QUIT_ARG:
            message = "RQT ERR\n";
            return message;
        case INVALID_DEBUG_ARG:
            message = "RDB ERR\n";
            return message;
        case ONGOING_GAME:
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
        case NO_ONGOING_GAME:
            message = "RQT NOK\n";
            return message;
        case NO_GAMES:
            message = "RST NOK\n";
            return message;
    }
    return NULL;
}

/**
 * Funcao que começa um jogo.
 * 
 * @param request o pedido dividido por palavras
 * @return mensagem a ser enviado para o cliente.
 */
string Server::start_game(vector<string> request) {
    string PLID = request[1];
    string max_playtime = request[2];
    string player_dir = "GAMES/" + PLID;
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    FILE *file;
    char buffer[MAX_BUFFER_SIZE];

    if (FindGame(PLID, NULL)) {
        time_t start_time, now = time(nullptr);
        int max_playtime;
        if ((file = fopen(file_name.c_str(), "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        fgets(buffer, 128, file);
        sscanf(buffer, "%*s %*s %*s %d %*s %*s %ld\n", &max_playtime, &start_time);

        int time_elapsed = (int) difftime(now, start_time);
        if (time_elapsed >= max_playtime) {
            end_game(request[1], GAME_TIMEOUT);
            fclose(file);
        } else {
            int n_tries = GetTrials(file).size();
            fclose(file);
            if (n_tries != 0) {
                return handle_error(ONGOING_GAME);
            }
        }
    }

    time_t now = time(nullptr);

    if ((file = fopen(file_name.c_str(), "w")) == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    create_dir(player_dir.c_str());

    const char valid_colors[] = {'R', 'G', 'B', 'Y', 'O', 'P'};
    string mode = "PLAY";
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


    sprintf(buffer, "%s %c %s %s %s %s %ld\n",
            PLID.c_str(),     
            mode[0],             
            code,             
            max_playtime.c_str(), 
            date,           
            time_str, 
            now);

    size_t n = fwrite(buffer, sizeof(char), strlen(buffer), file);
    if (n < strlen(buffer)) {
        fclose(file);
        fprintf(stderr, "Error writing into file.\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    string message = "RSG OK\n";
    if (verbose) {
       string client_ip = string(inet_ntoa(UDPsocket.client_addr.sin_addr));
        string client_port = std::to_string(ntohs(UDPsocket.client_addr.sin_port));
        string verbose_msg = "IP: " + client_ip + "\nport: " + client_port + "\nPLID: " + PLID + 
                             "\nRequest: Start (max_playtime=" + max_playtime + ")\n\n";
        sendMessage(1, verbose_msg, verbose_msg.size());
    }
    return message;
}

/**
 * Funcao que faz uma tentativa para advinhar a solucao.
 * 
 * @param request o pedido dividido por palavras
 * @return mensagem a ser enviado para o cliente.
 */
string Server::try_colors(vector<string> request) {
    string PLID = request[1];
    string C1 = request[2];
    string C2 = request[3];
    string C3 = request[4];
    string C4 = request[5];
    string nT = request[6];
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    char buffer[MAX_BUFFER_SIZE];
    FILE *file;
    string guess = C1 + C2 + C3 + C4;
    char target[MAX_COLOR_SIZE];
    int max_playtime, n_tries;
    time_t start_time;

    if ((file = fopen(file_name.c_str(), "a+")) == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }    

    fgets(buffer, MAX_BUFFER_SIZE, file);
    sscanf(buffer, "%*s %*s %s %d %*s %*s %ld\n", target, &max_playtime, &start_time);

    time_t now = time(nullptr);
    int time_elapsed = (int) difftime(now, start_time);
    if (time_elapsed >= max_playtime) {
        end_game(PLID, GAME_TIMEOUT);
        string message = handle_error(OUT_OF_TIME) + target[0] + " " + target[1] + " " + target[2] + " " + target[3] + "\n";
        return message;
    }

    if (!valid_color(request[2]) || !valid_color(request[3])
        || !valid_color(request[4]) || !valid_color(request[5])) {
            return handle_error(INVALID_TRY_ARG);
    }
    
    vector<TRIAL> trials = GetTrials(file);
    n_tries = trials.size();
    
    if ((n_tries == atoi(nT.c_str())) && (n_tries != 0)) {
        if (guess.compare(trials[n_tries - 1].guess)) {
            return handle_error(INVALID_TRIAL);
        } else {
            n_tries--;
        }
    } else if (n_tries == atoi(nT.c_str()) - 1) {
        for (TRIAL s: trials) {
            if (!s.guess.compare(guess)) {
                return handle_error(DUPLICATED_GUESS);
            }
        }
    } else {
        return handle_error(INVALID_TRIAL);
    }

    if ((file = fopen(file_name.c_str(), "a")) == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    int nB = 0;
    int nW = 0;

    const char valid_colors[] = {'R', 'G', 'B', 'Y', 'O', 'P'};
    int target_counts[6] = {0, 0, 0, 0, 0, 0};

    for (size_t i = 0; i < 4; ++i) {
        if (target[i] == guess[i]) {
            nB++;
        } else {
            int target_index = FindIndex(valid_colors, target[i], 6);
            target_counts[target_index]++;
        }
    }

    for (size_t i = 0; i < 4; ++i) {
        int guess_index = FindIndex(valid_colors, guess[i], 6);
        if (target[i] != guess[i] && target_counts[guess_index] > 0) {
            nW++;
            target_counts[guess_index]--;
        }
    }

    sprintf(buffer, "T: %s %d %d %d\n", guess.c_str(), nB, nW, time_elapsed);
    size_t n = fwrite(buffer, sizeof(char), strlen(buffer), file);
    if (n < strlen(buffer)) {
        fclose(file);
        fprintf(stderr, "Error writing into file.\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    string message = "RTR OK " + nT + " " + std::to_string(nB) + " " + std::to_string(nW) + "\n";
    if (verbose) {
        string client_ip = string(inet_ntoa(UDPsocket.client_addr.sin_addr));
        string client_port = std::to_string(ntohs(UDPsocket.client_addr.sin_port));
        string verbose_msg = "IP: " + client_ip + "\nport: " + client_port + "\nPLID: " + PLID + "\nRequest: Try " +
                             guess[0] + " " + guess[1] + " " + guess[2] + " " + guess[3] + ", nB = " +
                             std::to_string(nB) + ", nW = " + std::to_string(nW) + ", nT = " + nT + "\n\n";
        sendMessage(1, verbose_msg, verbose_msg.size());
    }

    if (nB == 4) {
        end_game(PLID, GAME_WIN);
    }

    if (n_tries == MAX_TRIALS - 1) {
        if (nB != 4) {
            string message = handle_error(OUT_OF_GUESSES) + target[0] + " " + target[1] + " " + target[2] + " " + target[3] + "\n";
            end_game(PLID, GAME_FAIL);
            return message;
        }
    }

    return message;
}

/**
 * Funcao que começa um jogo em modo debug.
 * 
 * @param request o pedido dividido por palavras
 * @return mensagem a ser enviado para o cliente.
 */
string Server::debug_mode(vector<string> request) {
    string PLID = request[1];
    string max_playtime = request[2];
    string player_dir = "GAMES/" + PLID;
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    char buffer[MAX_BUFFER_SIZE];
    FILE *file;

    if (FindGame(PLID, NULL)) {
        time_t start_time, now = time(nullptr);
        int max_playtime;
        if ((file = fopen(file_name.c_str(), "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        fgets(buffer, MAX_BUFFER_SIZE, file);
        sscanf(buffer, "%*s %*s %*s %d %*s %*s %ld\n", &max_playtime, &start_time);

        int time_elapsed = (int) difftime(now, start_time);
        if (time_elapsed >= max_playtime) {
            end_game(request[1], GAME_TIMEOUT);
            fclose(file);
        } else {
            if (VerifyOngoing(PLID)) {
                int n_tries = GetTrials(file).size();
                fclose(file);
                return handle_error(ONGOING_GAME);
            }
        }
    }

    if (!valid_color(request[3]) || !valid_color(request[4])
        || !valid_color(request[5]) || !valid_color(request[6])) {
            return handle_error(INVALID_DEBUG_ARG);
    }

    if ((file = fopen(file_name.c_str(), "w")) == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    time_t now = time(nullptr);

    create_dir(player_dir.c_str());

    string mode = "DEBUG";
    char code[MAX_COLOR_SIZE];
    code[0] = request[3][0];
    code[1] = request[4][0];
    code[2] = request[5][0];
    code[3] = request[6][0];
    code[4] = '\0';

    struct tm *local_time = localtime(&now);
    char date[MAX_DATE_SIZE], time_str[MAX_TIME_SIZE];
    strftime(date, sizeof(date), "%Y-%m-%d", local_time);   
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time); 

    sprintf(buffer, "%s %c %s %s %s %s %ld\n",
            PLID.c_str(),     
            mode[0],             
            code,             
            max_playtime.c_str(), 
            date,           
            time_str, 
            now);

    size_t n = fwrite(buffer, sizeof(char), strlen(buffer), file);
    if (n < strlen(buffer)) {
        fclose(file);
        fprintf(stderr, "Error writing into file.\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);

    string message = "RDB OK\n";
    if (verbose) {
        string client_ip = string(inet_ntoa(UDPsocket.client_addr.sin_addr));
        string client_port = std::to_string(ntohs(UDPsocket.client_addr.sin_port));
        string verbose_msg = "IP: " + client_ip + "\nport: " + client_port + "\nPLID: " + PLID + 
                             "\nRequest: Start (max_playtime=" + max_playtime + ", solution=" + string(code) + ")\n\n";
        sendMessage(1, verbose_msg, verbose_msg.size());
    }
    return message;
}

/**
 * Funcao que produz uma mensagem com as jogadas de um certo jogador.
 * 
 * @param plid player id
 * @return mensagem a ser enviado para o cliente
 */
string Server::show_trials(string plid) {
    string message;
    string Fdata;
    string trials = "";
    FILE *file;
    char file_name[64], buffer[MAX_BUFFER_SIZE], guess[MAX_COLOR_SIZE],
        Fdata_aux[MAX_BUFFER_SIZE], date[MAX_DATE_SIZE], time_str[MAX_TIME_SIZE];
    int max_playtime, nB, nW, time_elapsed, n_trials, time_left;
    time_t start_time;
    n_trials = 0;
    string file_n = "GAMES/GAME_" + plid + ".txt";

    if (FindGame(plid, NULL)) {
        time_t start_time, now = time(nullptr);
        int max_playtime;
        if ((file = fopen(file_n.c_str(), "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        fgets(buffer, MAX_BUFFER_SIZE, file);
        sscanf(buffer, "%*s %*s %*s %d %*s %*s %ld\n", &max_playtime, &start_time);
        fclose(file);

        int time_elapsed = (int) difftime(now, start_time);
        if (time_elapsed >= max_playtime) {
            end_game(plid, GAME_TIMEOUT);
        }
    }

    if (FindGame(plid, NULL)) {
        if ((file = fopen(file_n.c_str(), "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        message = "RST ACT STATE_" + plid + ".txt ";
        Fdata = "     Active game found for player " + plid + "\n";

        fgets(buffer, MAX_BUFFER_SIZE, file);
        sscanf(buffer, "%*s %*s %*s %d %s %s %ld\n", &max_playtime, date, time_str, &start_time);
        sprintf(Fdata_aux, "Game initiated: %s %s with %d seconds to be completed\n\n", 
                    date, time_str, max_playtime);
        Fdata += string(Fdata_aux);

        while (fgets(buffer, MAX_BUFFER_SIZE, file)) {
            if (buffer[0] == 'T') {
                sscanf(buffer, "T: %s %d %d %d\n", guess, &nB, &nW, &time_elapsed);
                sprintf(Fdata_aux, "Trial: %s, nB: %d, nW: %d at %3ds\n", guess, nB, nW, time_elapsed);
                trials += string(Fdata_aux);
                n_trials++;
            }
        }

        time_t now = time(nullptr);
        int time_elapsed = (int) difftime(now, start_time);
        time_left = max_playtime - time_elapsed;
        if (n_trials == 0) {
            Fdata += "     Game started - no transactions found\n\n  -- " + std::to_string(time_left) +  " seconds remaining to be completed --\n\n";
        } else {
            Fdata += "     --- Transactions found: " + std::to_string(n_trials) + " ---\n\n" + trials +  "\n  -- " + std::to_string(time_left) +  " seconds remaining to be completed --\n\n";
        }

    } else if (FindLastGame((char *) plid.data(), file_name)) {

        char termination, mode;
        char target[MAX_COLOR_SIZE], end_date[MAX_DATE_SIZE], end_time[MAX_TIME_SIZE];
        string termination_str;
        int duration;
        
        if ((file = fopen(file_name, "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        message = "RST FIN STATE_" + plid + ".txt ";
        Fdata = "     Last finalized game for player " + plid + "\n";
        fgets(buffer, MAX_BUFFER_SIZE, file);
        sscanf(buffer, "%*s %c %s %d %s %s %ld\n", &mode, target, &max_playtime, date, time_str, &start_time);
        sprintf(Fdata_aux, "Game initiated: %s %s with %d seconds to be completed\n", 
                    date, time_str, max_playtime);
        if (mode == 'P') {
            Fdata += string(Fdata_aux) + "Mode: PLAY  Secret code: " + string(target) + "\n\n";
        } else {
            Fdata += string(Fdata_aux) + "Mode: DEBUG Secret code: " + string(target) + "\n\n";
        }

        while (fgets(buffer, MAX_BUFFER_SIZE, file)) {
            if (buffer[0] == 'T') {
                sscanf(buffer, "T: %s %d %d %d\n", guess, &nB, &nW, &time_elapsed);
                sprintf(Fdata_aux, "Trial: %s, nB: %d, nW: %d at %3ds\n", guess, nB, nW, time_elapsed);
                trials += string(Fdata_aux);
                n_trials++;
            } else {
                sscanf(buffer, "%s %s %d\n", end_date, end_time, &duration);
            }
        }
        fclose(file);

        termination = file_name[29];
        switch(termination) {
            case 'W':
                termination_str = "WIN";
                break;
            case 'F':
                termination_str = "FAIL";
                break;
            case 'Q':
                termination_str = "QUIT";
                break;
            case 'T':
                termination_str = "TIMEOUT";
                break;
        }

        time_t now = time(nullptr);
        int time_elapsed = (int) difftime(now, start_time);
        time_left = max_playtime - time_elapsed;
        if (n_trials == 0) {
            Fdata += "     Game started - no transactions found\n";
        } else {
            Fdata += "     --- Transactions found: " + std::to_string(n_trials) + " ---\n" + trials;
        }
        Fdata += "     Termination: " + termination_str + " at " + string(end_date) + " " + string(end_time) +
                 ", Duration: " + std::to_string(duration) + "s\n\n";

    } else {
        return handle_error(NO_GAMES);
    }
    
    if (verbose) {
        string client_ip = string(inet_ntoa(TCPsocket.client_addr.sin_addr));
        string client_port = std::to_string(ntohs(TCPsocket.client_addr.sin_port));
        string verbose_msg = "IP: " + client_ip + "\nport: " + client_port + "\nPLID: " + plid + 
                             "\nRequest: Show_trials\n\n";
        sendMessage(1, verbose_msg, verbose_msg.size());
    }
    message += std::to_string(strlen(Fdata.c_str())) + " " + Fdata;
    return message;

}

/**
 * Funcao que mostra os 10 jogadores com maior pontuacao.
 * 
 * @return mensagem a ser enviado para o cliente
 */
string Server::scoreboard() {
    string message;
    string top_score = "";
    string Fdata = "-------------------------------- TOP 10 SCORES --------------------------------\n\n                 SCORE PLAYER     CODE    NO TRIALS   MODE\n\n";
    char fname[24] = "TOPSCORES_XXX.txt ";
    sprintf(fname, "TOPSCORES_%07d.txt", topscore_requests);
    if (!FindTopScores(top_score)) {
        message = "RSS EMPTY";
        return message;
    }
    Fdata += top_score + "\n\n\n";
    message = string("RSS OK ") + fname + " " + std::to_string(strlen(Fdata.c_str())) + " " + Fdata;

    if (verbose) {
        string client_ip = string(inet_ntoa(TCPsocket.client_addr.sin_addr));
        string client_port = std::to_string(ntohs(TCPsocket.client_addr.sin_port));
        string verbose_msg = "IP: " + client_ip + "\nport: " + client_port + "\nRequest: Scoreboard\n\n";
        sendMessage(1, verbose_msg, verbose_msg.size());
    }

    if (++topscore_requests == 10000000) {
        topscore_requests = 0;
    }
    return message;
}

/**
 * Funcao que termina um jogo.
 * 
 * @param request o pedido dividido por palavras
 * @return mensagem a ser enviado para o cliente.
 */
string Server::quit_game(vector<string> request) {
    FILE *file;
    string PLID = request[1];
    string file_name = "GAMES/GAME_" + PLID + ".txt";
    char buffer[MAX_BUFFER_SIZE], target[MAX_COLOR_SIZE];

    if (FindGame(PLID, NULL)) {
        time_t start_time, now = time(nullptr);
        int max_playtime;
        if ((file = fopen(file_name.c_str(), "r")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        fgets(buffer, MAX_BUFFER_SIZE, file);
        sscanf(buffer, "%*s %*s %*s %d %*s %*s %ld\n", &max_playtime, &start_time);
        fclose(file);

        int time_elapsed = (int) difftime(now, start_time);
        if (time_elapsed >= max_playtime) {
            end_game(PLID, GAME_TIMEOUT);
            return handle_error(NO_ONGOING_GAME);
        }
    } else {
        return handle_error(NO_ONGOING_GAME);
    }

    if ((file = fopen(file_name.c_str(), "r")) == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    fgets(buffer, MAX_BUFFER_SIZE, file);
    sscanf(buffer, "%*s %*s %s", target);
    fclose(file);

    end_game(PLID, GAME_QUIT);

    if (verbose) {
        string client_ip = string(inet_ntoa(UDPsocket.client_addr.sin_addr));
        string client_port = std::to_string(ntohs(UDPsocket.client_addr.sin_port));
        string verbose_msg = "IP: " + client_ip + "\nport: " + client_port + "\nPLID: " + PLID + "\nRequest: Quit\n\n";
        sendMessage(1, verbose_msg, verbose_msg.size());
    }
    string message = "RQT OK " + std::string(1, target[0]) + " " + std::string(1, target[1]) + " " + std::string(1, target[2]) + " " + std::string(1,target[3]) + "\n";
    return message;
}

/**
 * Funcao que termina um jogo em curso, atualizando o seu estado localmente.
 * 
 * @param plid player id
 * @param status modo como terminou o jogo
 * @return mensagem a ser enviado para o cliente.
 */
void Server::end_game(string plid, int status) {
    FILE *file;
    int max_playtime, score = 0;
    time_t start_time;
    char buffer[MAX_BUFFER_SIZE], target[MAX_COLOR_SIZE], mode;
    string s_status;
    string score_file_name;
    string file_name = "GAMES/GAME_" + plid + ".txt";
    if ((file = fopen(file_name.c_str(), "a+")) == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }
    fgets(buffer, MAX_BUFFER_SIZE, file);
    sscanf(buffer, "%*s %c %s %d %*s %*s %ld\n", &mode, target, &max_playtime, &start_time);
    vector<TRIAL> trials = GetTrials(file);
    int n_tries = trials.size();

    time_t now = time(nullptr);
    struct tm *local_time = localtime(&now);
    char date[MAX_DATE_SIZE], time_str[MAX_TIME_SIZE];
    int time_elapsed = std::min((int) difftime(now, start_time), max_playtime);
    strftime(date, sizeof(date), "%Y-%m-%d", local_time);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    sprintf(buffer, "%s %s %d\n", date, time_str, time_elapsed);

    size_t n = fwrite(buffer, sizeof(char), strlen(buffer), file);
    if (n < strlen(buffer)) {
        fclose(file);
        fprintf(stderr, "Error writing into file.\n");
        exit(EXIT_FAILURE);
    }

    fclose(file);

    switch(status) {
        case GAME_WIN:
            s_status = "_W";
            score = getScore(n_tries);
            break;
        case GAME_FAIL:
            s_status = "_F";
            break;
        case GAME_QUIT:
            s_status = "_Q";
            break;
        case GAME_TIMEOUT:
            s_status = "_T";
            break;
    }

    char date_aux[9], time_aux[7];
    strftime(date_aux, sizeof(date_aux), "%Y%m%d", local_time);
    strftime(time_aux, sizeof(time_aux), "%H%M%S", local_time);

    string mode_str;
    if (mode == 'P') {
        mode_str = "PLAY";
    } else {
        mode_str = "DEBUG";
    }

    if (status == GAME_WIN) {
        char score_aux[MAX_SCORE_SIZE];
        sprintf(score_aux, "%03d", score);
        score_file_name = "SCORES/" + string(score_aux) + "_" + plid + "_" + string(date_aux) + "_" + string(time_aux) + ".txt";
        if ((file = fopen(score_file_name.c_str(), "w")) == NULL) {
            fprintf(stderr, "Error opening file.\n");
            exit(EXIT_FAILURE);
        }
        sprintf(buffer, "%03d %s %s %d %s\n", score, plid.c_str(), target, n_tries, mode_str.c_str());
        size_t n = fwrite(buffer, sizeof(char), strlen(buffer), file);
        if (n < strlen(buffer)) {
            fclose(file);
            fprintf(stderr, "Error writing into file.\n");
            exit(EXIT_FAILURE);
        }
        fclose(file);
    }

    string new_file_name = "GAMES/" + plid + "/" + string(date_aux) + "_" + string(time_aux) + s_status + ".txt";

    if (rename(file_name.c_str(), new_file_name.c_str()) < 0) {
        fprintf(stderr, "Error renaming file.\n");
        exit(EXIT_FAILURE);
    }

}

/**
 * Funcao que calcula a pontuacao de um jogo terminado com sucesso.
 * 
 * @param n_tries número de tentativas
 * @return pontuacao do jogo
 */
int Server::getScore(int n_tries) {

    return (MAX_TRIALS - (n_tries - 1)) * 100 / MAX_TRIALS;

}

/**
 * Funcao que retorna as jogadas de um ficheiro.
 * 
 * @param file ficheiro
 * @return vetor com as jogadas realizadas
 */
vector<TRIAL> Server::GetTrials(FILE *file) {
    int nB, nW, time;
    char buffer[MAX_BUFFER_SIZE], guess[MAX_COLOR_SIZE];
    vector<TRIAL> trials;
    bzero(buffer, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), file)) {
        if (buffer[0] == 'T') {
            sscanf(buffer, "T: %s %d %d %d\n", guess, &nB, &nW, &time);
            TRIAL trial;
            trial.guess = string(guess);
            trial.nB = nB;
            trial.nW = nW;
            trial.time = time;
            trials.push_back(trial);
        }
        bzero(buffer, sizeof(buffer));
    }
    return trials;
}

/**
 * Funcao que verifica se o jogador tem algum jogo em curso.
 * 
 * @param plid player id
 * @return Existe ou nao jogo em curso
 */
int Server::VerifyOngoing(string plid) {

    int line_count = 0;
    char buffer[MAX_BUFFER_SIZE], fname[24];
    string file_name;
    FILE* file;
    bzero(fname, sizeof(fname));
    if (!FindGame(plid, fname)) {
        return 0;
    }
    
    file_name = "GAMES/" + string(fname);
    file = fopen(file_name.c_str(), "r");
    if (file == nullptr) {
        fprintf(stderr, "Error opening file.\n");
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }

    fclose(file);

    if (line_count == 1) {
        return 0;
    }
    return 1;

}

/**
 * Funcao que verifica se o jogador tem um jogo comecado.
 * 
 * @param PLID player id
 * @param fname nome do ficheiro do jogador
 * @return Existe ou nao jogo comecado pelo jogador
 */
int Server::FindGame(string PLID, char *fname) {
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
            fname = (char *) file_name.c_str();
            found = 1;
            break;
        }
    }
    closedir(dir);
    return found;
}

/**
 * Funcao que retorna as jogadas de um ficheiro.
 * 
 * @param PLID player id
 * @param fname nome do ficheiro
 * @return Existe ou nao jogo terminado pelo jogador
 */
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
        while (n_entries--) {
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

/**
 * Funcao que encontra os 10 jogadores com a maior pontuacao.
 * 
 * @param message string com os 10 jogadores com a maior pontuacao
 * @return Sucesso ou nao
 */
int Server::FindTopScores(string& message) {
    struct dirent** filelist;
    int n_entries;
    int n_game = 0;
    int n_file = 0;
    char buffer[MAX_BUFFER_SIZE];
    FILE* fptr;
    vector<string> file_content;

    n_entries = scandir("SCORES/", &filelist, NULL, reverse_alphasort);
    if (n_entries <= 0) {
        fprintf(stderr, "scandir error\n");
        return 0;
    }
    else {
        while (n_file < 10 && n_file < n_entries) {
            if (filelist[n_file]->d_name[0] != '.') {
                string file_name = "SCORES/" + string(filelist[n_file]->d_name);
                if ((fptr = fopen(file_name.c_str(), "r")) == NULL) {
                        fprintf(stderr, "Error opening file.\n");
                        exit(EXIT_FAILURE);
                }
                n_game++;
                fgets(buffer, 128, fptr);
                file_content = split_line(buffer);

                if (!strcmp(file_content[4].c_str(), "P")) {
                    file_content[4] = string("PLAY");
                }
                else if (!strcmp(file_content[4].c_str(), "D")) {
                    file_content[4] = string("DEBUG");
                }

                sprintf(buffer, "%14d - %4s %7s %8s %8s %10s\n",
                    n_game,
                    file_content[0].c_str(),
                    file_content[1].c_str(),
                    file_content[2].c_str(),
                    file_content[3].c_str(),
                    file_content[4].c_str());

                message += string(buffer);

                free(filelist[n_file]);
                n_file++;
            }
            else {
                free(filelist[n_file]);
                n_file++;
            }
        }
    }
    free(filelist);
    message += "\0";
    return n_game;
}

int main(int argc, char** argv) {

    Server(argc, argv);

    return 0;
}