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
#include <iostream>
#include <vector>

using std::string;
using std::vector;

int check_start_input(vector<string> line);


#endif