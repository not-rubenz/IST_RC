#ifndef __H_UTILS
#define __H_UTILS

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <cerrno>
#include <algorithm>
#include <cctype>
#include "constants.hpp"

using std::string;
using std::vector;

vector<string> split_line(string line);
int sendTCP(int fd, string message, int size);
int receiveTCP(int fd, char* message, int size);
int FindIndex(const char array[], char value, int size);
int valid_PLID(string PLID);
int valid_time(string time);
int valid_color(string color);

#endif