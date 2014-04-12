#ifndef CHAT_H
#define CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
// mkfifo
#include <sys/stat.h>
#include <pthread.h> 
#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define CMD_LOGIN 1
#define CMD_REGISTER 2
#define CMD_EXIT 3
#define CMD_SEND_ID 4
#define CMD_SEND_ALL 5
#define CMD_HELP 6
#define CMD_LIST_CHATERS 9
#define CMD_SEND_CHATROOM 10
#define CMD_CREATE_CHATROOM 11
#define CMD_JOIN_CHATROOM 12
#define CMD_LIST_CHATROOMS 13
#define CMD_CLIENT_EXIT 14

#define LOGIN_SUCCESS 7
#define LOGIN_FAIL 8

#define CHATER_ONLINE 15
#define CHATER_OFFLINE 16

#define SERVER_FIFO "/tmp/linhaojie_2010159018_fifo"
#define CLIENT_FIFO "/tmp/client%d_fifo"

#define LOGIN_NO_SUCH_CHATER "no such chater"
#define LOGIN_PWD_ERROR "pwd error"
#define REGISTER_SUCCESS "register successful"
#define REGISTER_FAIL "register fail"
#define CHATERS_DB "chater_config.db"

#define CHK(RESULT, EXPRESSION, ERROR_MSG) if((RESULT) != (EXPRESSION)) {perror(ERROR_MSG); exit(-1);}
#define CHK_E(RESULT, EXPRESSION, ERROR_MSG) if((RESULT) == (EXPRESSION)) {perror(ERROR_MSG); exit(-1);}
#define CHK_R(RESULT, EXPRESSION, ERROR_MSG) if((RESULT) == (EXPRESSION)) {perror(ERROR_MSG); return;}
#define CHK_R_N(RESULT, EXPRESSION, ERROR_MSG) if((RESULT) == (EXPRESSION)) { perror(ERROR_MSG); return NULL;}

// default chatrooms
#define ROOMS_SIZE 4
char *c_rooms[ROOMS_SIZE] = { "FAMILY", "LIFE", "TECHNOLOGY", "STUDY" };

typedef struct {
	int cmd;      	// cmd id: send,send all
	pid_t tar_id;   // want to send target id
	pid_t pid;      // self id
	char msg[500];	// message
	char send_name[50];
	char recv_name[50];
	char pwd[50];
} MSG, *MSG_PTR;

typedef struct {
	// login LOGIN_SUCCESS or LOGIN_FAIL
	int login_s_f;
	char msg[50];
} RESPONSE, *RESPONSE_PTR;

// use info to store the online client
typedef struct info {
	char cli_name[50];
	char pwd[50];
	pid_t pid;
	int state;
	char c_room[50];
	struct info* next;
} CLIENT, *CLIENT_PTR;

typedef struct {
	double _count;
	struct info *clients;
} CURRENT_ONLINE, *CURRENT_ONLINE_PTR;

// the chatrooms info
typedef struct chatroom {
	char c_name[50];
	struct chatroom *next;
} C_ROOMS, *C_ROOMS_PTR;

#endif
