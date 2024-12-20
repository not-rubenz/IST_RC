#ifndef __H_CONSTANTS
#define __H_CONSTANTS

#define GROUP_NUMBER "36"
#define GSIP_DEFAULT "127.0.0.1"
#define GSPORT_DEFAULT "58036"

#define MAX_UDPBUFFER_SIZE 128
#define MAX_TCPBUFFER_SIZE 2560
#define MAX_PLAYTIME 600
#define MAX_COMMAND_SIZE 4
#define MAX_STATUS_SIZE 4
#define MAX_PLID_SIZE 7
#define MAX_COLOR_SIZE 5
#define MAX_BUFFER_SIZE 128
#define MAX_DATE_SIZE 11
#define MAX_TIME_SIZE 9
#define MAX_SCORE_SIZE 4
#define TIMEOUT_SEC 5

///////////////////////////////////////////////////
#define START "start"
#define TRY "try"
#define SHOW_TRIAL "show_trial"
#define ST "st"
#define SCOREBOARD "scoreboard"
#define SB "sb"
#define QUIT "quit"
#define EXIT "exit"
#define DEBUG "debug"
///////////////////////////////////////////////////
#define START_REQUEST "SNG"
#define TRY_REQUEST "TRY"
#define SHOW_TRIAL_REQUEST "STR"
#define SCOREBOARD_REQUEST "SSB"
#define QUIT_REQUEST "QUT"
// #define EXIT_REQUEST ""
#define DEBUG_REQUEST "DBG"

/////////////////////////////////////////////////
#define INVALID_INPUT 100
#define INVALID_PLID 101
#define INVALID_CMD_SYNTAX 102
#define INVALID_START_ARG 103
#define INVALID_TRY_ARG 104
#define INVALID_DEBUG_ARG 105
#define ONGOING_GAME 106
#define DUPLICATED_GUESS 107
#define INVALID_TRIAL 108
#define OUT_OF_CONTEXT 109
#define OUT_OF_GUESSES 110
#define OUT_OF_TIME 111
#define INVALID_QUIT_ARG 113
#define NO_ONGOING_GAME 114
#define INVALID_ST_TAG 115
#define NO_GAMES 116


#define GAME_WIN 200
#define GAME_FAIL 201
#define GAME_QUIT 202
#define GAME_TIMEOUT 203
#define MAX_TRIALS 8


#endif