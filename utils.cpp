#include "utils.hpp"


vector<string> split_line(string line) {
    std::stringstream ss(line);
    string word;
    vector<string> words;
    
    while (ss >> word) {
        words.push_back(word);
    }
    
    return words;
}

int sendTCP(int fd, string message, int nbytes){
    ssize_t nleft, n;
    int nwritten = 0;
    char * ptr, buffer[1024];

    ptr = strcpy(buffer, message.c_str());

    nleft = nbytes;
    while(nleft > 0){
        n = write(fd, ptr, nleft);
        if(n <= 0){
            fprintf(stderr, "Unable to send message, please try again!\n");
            return -1;
        }
        nleft -= n;
        ptr += n;
        nwritten += n;
    }
    return nwritten;
}

int receiveTCP(int fd, char * message, int nbytes){
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