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

int receiveTCP(int fd, char *message, int size) {
    int bytes_read = 0;
    char *ptr = message;

    while (bytes_read < size) {
        ssize_t n = read(fd, ptr, size - bytes_read);
        if (n == -1) {
            if (errno == EINTR) {
                continue; 
            } else {
                fprintf(stderr, "Error while reading message!\n");
                return -1;
            }
        }

        if (n == 0) {

            break;
        }

        bytes_read += n;
        ptr += n;
    }
    return bytes_read;
}

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

bool isNumber(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

int valid_PLID(string PLID) {
    return PLID.length() == 6;
}

int valid_time(string time) {
    return isNumber(time) && stoi(time) <= MAX_PLAYTIME;
}

int valid_color(string color) {
    if (color.length() != 1) return 0;

    char c = color[0];
    return c == 'R' || c == 'G' || c == 'B' || c == 'Y' || c == 'O' || c == 'P';
}
