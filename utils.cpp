#include "utils.hpp"

/**
 * Recebe um string e divide em palavras.
 * 
 * @param line linha de comando
 * @return linha dividida em palavras
 */
vector<string> split_line(string line) {
    std::stringstream ss(line);
    string word;
    vector<string> words;
    
    while (ss >> word) {
        words.push_back(word);
    }
    
    return words;
}

/**
 * Envia mensagem.
 * 
 * @param fd file descriptor
 * @param message mensagem a ser enviada
 * @param size tamanho da mensagem
 * @return numero de caracteres enviados
 */
int sendMessage(int fd, string message, int size){
    int bytes_written = 0;
    char *ptr, buffer[1024];

    ptr = strcpy(buffer, message.c_str());
    while(bytes_written < size){
        ssize_t n = write(fd, ptr, size - bytes_written);
        if(n == -1){
            if (errno == EINTR) {
                continue;
            } else {
                fprintf(stderr, "Unable to send message, please try again!\n");
                return -1;
            }
        }
        ptr += n;
        bytes_written += n;
    }
    return bytes_written;
}

/**
 * Recebe mensagem.
 * 
 * @param fd file descriptor
 * @param message mensagem a ser recebida
 * @param size tamanho da mensagem
 * @return numero de caracteres recebidos
 */
int receiveMessage(int fd, char * message, int nbytes) {
    ssize_t nleft, nread;
    char *ptr;
    nleft = nbytes;
    ptr = message;

    while(nleft > 0){
        nread = read(fd, ptr, nleft);
        if(nread == -1){
            break;  
        }
        else if(nread == 0) break;
        nleft -= nread;
        ptr += nread;
    }
    nread = nbytes - nleft;
    return nread;
}

/**
 * Recebe palavra.
 * 
 * @param fd file descriptor
 * @param message palavra a ser recebida
 * @param size tamanho da palavra
 * @return numero de caracteres recebidos
 */
int receiveWord(int fd, char * message, int nbytes) {
    char buffer[2];
    int bytes_read = 0;
    int i;
    for(i = 0; i < nbytes; i++){
        if(receiveMessage(fd, buffer, 1) == -1){
            return -1;
        }
        bytes_read++;
        if(buffer[0] == ' ' || buffer[0] == '\n'){
            break;
        }
        message[i] = buffer[0];
    }

    message[i] = '\0';
    return bytes_read;
}

/**
 * Funcao que retorna o indice de value em um array
 * 
 * @param array
 * @param value valor a ser encontrado
 * @param size tamanaho do array
 * @return indice ou -1 em caso de insucesso
 */
int FindIndex(const char array[], char value, int size) {
    int index = 0;
    while (index < size) {
        if (array[index] == value) {
            return index;
        }
        ++index;
    }
    return -1;
}

/**
 * Verifica se a string e numero
 * 
 * @param string
 * @return True ou False
 */
bool isNumber(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

/**
 * Verifica se a string e player id valido
 * 
 * @param PLID
 * @return 1 ou 0
 */
int valid_PLID(string PLID) {
    return PLID.length() == 6;
}

/**
 * Verifica se a string e tempo valido
 * 
 * @param time
 * @return 1 ou 0
 */
int valid_time(string time) {
    return isNumber(time) && stoi(time) <= MAX_PLAYTIME;
}

/**
 * Verifica se a string e cor valida
 * 
 * @param color
 * @return 1 ou 0
 */
int valid_color(string color) {
    if (color.length() != 1) return 0;

    char c = color[0];
    return c == 'R' || c == 'G' || c == 'B' || c == 'Y' || c == 'O' || c == 'P';
}

/**
 * Verifica se a string e port valido
 * 
 * @param port_str
 * @return 1 ou 0
 */
int isPort(string port_str) {
    if (!isNumber) {
        return 0;
    }
    int port = stoi(port_str);
    return port >= 1 && port <= 65535;
}

/**
 * Algoritmo de organiza a diretoria ao contrario
 * 
 * @param a
 * @param b
 * @return 1 ou 0
 */
int reverse_alphasort(const struct dirent** a, const struct dirent** b) {
    return strcmp((*b)->d_name, (*a)->d_name); // Swap a and b
}
