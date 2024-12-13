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

int sendTCP(int fd, string message, int size){
    int bytes_written = 0;
    char *ptr, buffer[1024];

    ptr = strcpy(buffer, message.c_str());
    while(bytes_written < size){
        size_t n = write(fd, ptr, size - bytes_written);
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

int receiveTCP(int fd, char * message, int size){
    size_t bytes_read = 0;
    char *ptr;

    ptr = message;

    while(bytes_read < size){
        ssize_t n = read(fd, ptr, size - bytes_read);
        if(n == -1){
            if (errno == EINTR) {
                continue;
            } else {
                fprintf(stderr, "Error while reading message!\n");
                return -1;
            }
        }
        
        if(n == 0) {
            break;
        }

        bytes_read += n;
        ptr += n;
    }
    return bytes_read;
}