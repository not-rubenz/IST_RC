#include "utils.hpp"

int check_start_input(vector<string> line) {
    if (line[1].length() != 6) {
        fprintf(stderr, "Unable to create socket.\n");
    }
    else if (line.size() != 3) {
        fprintf(stderr, "Unable to create socket.\n");
    }

    try {
        line[2] = stoi(line[2]);
    }
    catch (std::invalid_argument) {fprintf(stderr, "Unable to create socket.\n");}
}