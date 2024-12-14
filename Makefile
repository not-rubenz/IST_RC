CC     = g++
# -Wall  - this flag is used to turn on most compiler warnings
# CFLAGS = -Wall

.PHONY: all clean

all: player server

player: player.cpp utils.o
	$(CC) $(CFLAGS) -o player player.cpp utils.o

utils.o: utils.cpp constants.hpp
	$(CC) $(CFLAGS) -c -o utils.o utils.cpp

server: server.cpp utils.o
	$(CC) $(CFLAGS) -o server server.cpp utils.o
	
clean:
	rm -f server player *.o