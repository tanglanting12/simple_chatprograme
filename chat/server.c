/**
 * server 
 * author: jayhoo
 */
#include "chat.h"

/*====================================================================*/
// p_head: point to the head of the client info
// p_tail: point to the tail of the client info
// p_curr: point to the current new client info
// ser_fd: the server fifo fd
/*====================================================================*/
CLIENT_PTR p_head;
CLIENT_PTR p_tail;

C_ROOMS_PTR room_p_head;
C_ROOMS_PTR room_p_tail;

MSG msg;
int ser_fd;

/*====================================================================*/
// pre declaration
/*====================================================================*/
void  init(int* fd);
void* thread_proc(void *info);
void  login(MSG_PTR p_msg);
void  chater_regist(MSG_PTR p_msg);
int   client_fd(int* fd, int pid); 	// init fd and return fd
void  send_msg(MSG_PTR p_msg);
void  send_msg_in(MSG_PTR p_msg, int flag, char* c_room);
void  list_chaters(MSG_PTR p_msg);
void  client_exit(MSG_PTR p_msg);
void  chatroom_proc(MSG_PTR p_msg);
int   get_online_chaters();
void  sign_kill(int signo);
void  clear();

/*===========================*/
// just for test
/*===========================*/
void _print()
{
	CLIENT_PTR p_curr = p_head;
	C_ROOMS_PTR room_p_curr = room_p_head;
	while(p_curr)
	{
		printf("[CLIENT]:name:%s pwd:%s\n", p_curr->cli_name, p_curr->pwd);
		p_curr = p_curr->next;
	}
	while(room_p_curr)
	{
		printf("[CHATROOM]:%s\n", room_p_curr->c_name);
		room_p_curr = room_p_curr->next;
	}
}

/*========================================================*/
// return online chaters's count
/*=========================================================*/
int get_online_chaters()
{
	int _count = 0;
	CLIENT_PTR p_curr = p_head;
	while(p_curr)
	{
		if(p_curr->state == CHATER_ONLINE) ++_count;
		p_curr = p_curr->next;
	}
	return _count;
}

/*====================================================================*/
// init function
// fd: server fifo fd
// first to init the data, include to read the file to gain client info
/*====================================================================*/
void init(int* fd)
{
	CLIENT_PTR p_curr = NULL;
	C_ROOMS_PTR room_p_curr = NULL;
	char buffer[50] = {'\0'};// must be init or segmentation fault (core dumped)
	char *tok;
	int i;
	// the third parm is the error msg
	CHK(0, mkfifo(SERVER_FIFO, 0777), "can not create server fifo");
	CHK(0, access(SERVER_FIFO, F_OK), "asscess server fifo fail");
	CHK_E(-1, *fd = open(SERVER_FIFO, O_RDONLY | O_NONBLOCK), "open server fifo error");
	// first process the client info init
	FILE* fp = fopen("chater_config.db", "a+");
	CHK_E(NULL, fp, "can not open chater_db");
	// seek to the begin of the file
	// the file is line with 'chater|pwd|'
	fseek(fp, 0, SEEK_SET);
	while(fgets(buffer, 50, fp) != NULL)
	{
		// fgets will return end-of-file or newline character 
		p_curr = malloc(sizeof(CLIENT));
		p_curr->state = CHATER_OFFLINE;
		p_curr->next = NULL;
		tok = strtok(buffer, "|");
		strncpy(p_curr->cli_name, tok, strlen(tok));
		tok = strtok(NULL, "|");
		strncpy(p_curr->pwd, tok, strlen(tok));
		// client link
		if(p_head)
		{	
			p_tail->next = p_curr;
			p_tail = p_curr;
		}
		else 
		{	
			p_head = p_tail = p_curr;
		}
		bzero(buffer, strlen(buffer));
	}
	fclose(fp);
	// init the chatroom
	room_p_head = room_p_tail = room_p_curr;
	for(i = 0; i < ROOMS_SIZE; ++i)
	{
		room_p_curr = malloc(sizeof(C_ROOMS));
		bzero(room_p_curr, sizeof(C_ROOMS));
		room_p_curr->next = NULL;
		strncpy(room_p_curr->c_name, c_rooms[i], strlen(c_rooms[i]));
		if(room_p_head)
		{
			room_p_tail->next = room_p_curr;
			room_p_tail = room_p_curr;
		}
		else 
		{
			room_p_head = room_p_tail = room_p_curr;
		}
	}
	return;
}

/*============================================================*/
// get the client fd
// fd: 
// pid: client pid
/*============================================================*/
int client_fd(int* fd, int pid)
{
	char cli_name[50] = {'\0'};
	sprintf(cli_name, CLIENT_FIFO, pid);
	if((*fd = open(cli_name, O_WRONLY | O_NONBLOCK)) == -1)
	{
		return -1;
	}
	return *fd;
}

/*============================================================*/
// login funtion
// p_msg: the client post data
/*============================================================*/
void login(MSG_PTR p_msg)
{
	int cli_fd, flag = 0;
	RESPONSE response;
	char buffer[50] = {'\0'};
	CHK_R(-1, client_fd(&cli_fd, p_msg->pid), "login: client fifo cannot open for write");
	bzero(&response, sizeof(response));
	CLIENT_PTR p_curr = p_head;
	// check the chater's login info
	while(p_curr)
	{
		flag = 1;
		if(!strcmp(p_curr->cli_name, p_msg->send_name))
		{
			if(!strcmp(p_curr->pwd, p_msg->pwd)) 
			{
				response.login_s_f = LOGIN_SUCCESS;
				p_curr->state = CHATER_ONLINE;
				// if the client login success, save the client pid
				p_curr->pid = p_msg->pid;
				write(cli_fd, &response, sizeof(RESPONSE));
				close(cli_fd);
				bzero(p_msg->msg, strlen(p_msg->msg));
				sprintf(p_msg->msg, "%s just Login, total online chaters is %d", p_msg->send_name, get_online_chaters());
				send_msg_in(p_msg, CMD_SEND_ALL, NULL);
				break;
			}
			else 
			{
				// pwd error
				response.login_s_f = LOGIN_FAIL;
				strcpy(response.msg, LOGIN_PWD_ERROR);
			}
			break;
		}
		p_curr = p_curr->next;
	}

	// no such chater
	if(!p_head || (!p_curr && flag))
	{
		response.login_s_f = LOGIN_FAIL;
		strcpy(response.msg, LOGIN_NO_SUCH_CHATER);
	}
	write(cli_fd, &response, sizeof(RESPONSE));
	close(cli_fd);
}

/*============================================================*/
// chater regist funciton
// p_msg: the client post data
/*============================================================*/
void chater_regist(MSG_PTR p_msg)
{
	int cli_fd;
	RESPONSE response;
	client_fd(&cli_fd, p_msg->pid);
	CLIENT_PTR p_curr = malloc(sizeof(CLIENT));
	bzero(p_curr, sizeof(CLIENT));
	p_curr->next = NULL;
	p_curr->state = CHATER_OFFLINE;
	strncpy(p_curr->cli_name, p_msg->send_name, strlen(p_msg->send_name));
	strncpy(p_curr->pwd, p_msg->pwd, strlen(p_msg->pwd));
	// to link the client
	if(p_head)
	{
		p_tail->next = p_curr;
		p_tail = p_curr;
	}
	else 
	{
		p_head = p_tail = p_curr;
	}
	// to save the msg in CHATERS_DB
	FILE* fp = fopen(CHATERS_DB, "a+");
	bzero(&response, sizeof(RESPONSE));
	if(fp != NULL)
	{
		fprintf(fp, "%s|%s|\n", p_msg->send_name, p_msg->pwd);
		strncpy(response.msg, REGISTER_SUCCESS, strlen(REGISTER_SUCCESS));
	}
	else 
	{	
		strncpy(response.msg, REGISTER_FAIL, strlen(REGISTER_FAIL));
	}
	fclose(fp);
	write(cli_fd, &response, sizeof(RESPONSE));
	close(cli_fd);
}

/*====================================================================*/
// send the msg to online chater
/*====================================================================*/
void send_msg(MSG_PTR p_msg)
{
	send_msg_in(p_msg, p_msg->cmd, NULL);
}

/*====================================================================*/
// send msg to online chaters
// flag: CMD_SNED_ID or CMD_SEND_ALL
/*====================================================================*/
void send_msg_in(MSG_PTR p_msg, int flag, char* c_room)
{
	int cli_fd;
	CLIENT_PTR p_curr = p_head;

	if(CMD_SEND_ID == flag)
	{
		while(p_curr && strcmp(p_msg->recv_name, p_curr->cli_name))		
		{
			p_curr = p_curr->next;
		}
		if(p_curr && (p_curr->state == CHATER_ONLINE))
		{
			// find the receive chater and send the msg
			client_fd(&cli_fd, p_curr->pid);
			p_msg->tar_id = p_curr->pid;
			write(cli_fd, p_msg, sizeof(MSG));
			printf("%d:%s -> %d:%s=%s\n", p_msg->pid, p_msg->send_name, p_msg->tar_id, 
					p_msg->recv_name, p_msg->msg);
		}
		else 
		{
			// find but chater not online or can't find the chater
		}
	}
	else if(CMD_SEND_ALL == flag)
	{		
		while(p_curr)
		{
			if(p_curr->state == CHATER_ONLINE)
			{
				client_fd(&cli_fd, p_curr->pid);
				p_msg->tar_id = p_curr->pid;
				if(c_room != NULL && !strcmp(p_curr->c_room, c_room))
				{
					write(cli_fd, p_msg, sizeof(MSG));
				}
				else if(c_room == NULL)
				{
					write(cli_fd, p_msg, sizeof(MSG));
				}
				close(cli_fd);
			}
			p_curr = p_curr->next;
		}
	}
	close(cli_fd);
}

/*====================================================================*/
// list all chaters to client
/*====================================================================*/
void list_chaters(MSG_PTR p_msg)
{
	int cli_fd;
	client_fd(&cli_fd, p_msg->pid);
	bzero(p_msg->msg, strlen(p_msg->msg));
	CLIENT_PTR p_curr = p_head;
	// the msg start with '*'
	strcat(p_msg->msg, "*");	
	while(p_curr)
	{
		if(p_curr->state == CHATER_ONLINE)
		{
			strcat(p_msg->msg, p_curr->cli_name);
			strcat(p_msg->msg, "*");
		}
		p_curr = p_curr->next;
	}
	printf("%d's Client To List the chaters %s\n", p_msg->pid, p_msg->msg);
	write(cli_fd, p_msg, sizeof(MSG));
	close(cli_fd);
}

/*===========================================================================*/
// enable client to join the chatroom
// to join first to list the chatrooms
// that can enable the client to choose the chatrooms
/*============================================================================*/
void chatroom_proc(MSG_PTR p_msg)
{	
	int cli_fd;

	MSG _msg;
	bzero(&_msg, sizeof(_msg));

	C_ROOMS_PTR p_room = room_p_head;
	CLIENT_PTR p_client = p_head;
	client_fd(&cli_fd, p_msg->pid);
	if(CMD_LIST_CHATROOMS == p_msg->cmd)
	{
		bzero(p_msg->msg, strlen(p_msg->msg));
		strcat(p_msg->msg, "*"); // enable client to split and print by '[**]'

		while(p_room)
		{
			strcat(p_msg->msg, p_room->c_name);
			strcat(p_msg->msg, "*");
			p_room = p_room->next;
		}
	}
	else if(CMD_JOIN_CHATROOM == p_msg->cmd) 
	{
		// if the chatroom exit
		while(p_room && strcmp(p_msg->msg, p_room->c_name))
			p_room = p_room->next;
		if(!p_room) 
		{
			bzero(p_msg->msg, strlen(p_msg->msg));
			strcpy(p_msg->msg, "please input the right chatroom's name");
		}
		else 
		{
			// update the chater's info
			while(p_client)
			{
				if(!strcmp(p_msg->send_name, p_client->cli_name))
				{
					bzero(p_client->c_room, strlen(p_client->c_room));
					strcpy(p_client->c_room, p_msg->msg);
					strcpy(p_msg->msg, "join success");
					break;
				}
				p_client = p_client->next;
			} // while
		} // else
	} 
	else if(CMD_CREATE_CHATROOM == p_msg->cmd)
	{
		p_room = malloc(sizeof(C_ROOMS));
		bzero(p_room, sizeof(C_ROOMS));
		strncpy(p_room->c_name, p_msg->msg, strlen(p_msg->msg));
		room_p_tail->next = p_room;
		room_p_tail = p_room;
		bzero(p_msg->msg, sizeof(p_msg->msg));
		strcpy(p_msg->msg, "Create rooms success!");
	}
	else if(CMD_SEND_CHATROOM == p_msg->cmd)
	{
		// p_msg->recv_name: the chatroom name
		send_msg_in(p_msg, CMD_SEND_ALL, p_msg->recv_name);
		bzero(p_msg->msg, strlen(p_msg->msg));
		strcpy(p_msg->msg, "Send to all chatroom's menber success!");
	}
	printf("%s: %s\n", p_msg->send_name, p_msg->msg);
	write(cli_fd, p_msg, sizeof(MSG));
	close(cli_fd);
}

/*===========================================================================*/
// client exit send the message to all the online client
/*============================================================================*/
void client_exit(MSG_PTR p_msg)
{
	char ch[50] = { '\0' };
	CLIENT_PTR p_curr = p_head;
	bzero(&msg, sizeof(MSG));
	while(p_curr)
	{
		if(!strcmp(p_curr->cli_name, p_msg->send_name))
		{
			p_curr->pid = -1;
			p_curr->state = CHATER_OFFLINE;
			msg.cmd = CMD_SEND_ALL;
			sprintf(ch, "Client %s have exited", p_msg->send_name);
			strncpy(msg.msg, ch, strlen(ch));
			send_msg(&msg);
			break;
		}
		p_curr = p_curr->next;
	}
}

/*====================================================================*/
// to catch the kill action 
// :in order to unlik fifo
/*====================================================================*/
void sign_kill(int signo)
{
	clear();
}


/*====================================================================*/
// clear function: to free the memory
/*====================================================================*/
void clear()
{
	CLIENT_PTR p_curr = NULL;
	C_ROOMS_PTR room_p_curr = NULL;
	CHK(0, unlink(SERVER_FIFO), "unlink server fifo fail");
	while(p_head)
	{
		p_curr = p_head;
		p_head = p_head->next;
		free(p_curr);
	}
	p_head = p_curr = p_tail = NULL;
	while(room_p_head)
	{
		room_p_curr = room_p_head;
		room_p_head = room_p_head->next;
		free(room_p_curr);
	}
	close(ser_fd);
	unlink(SERVER_FIFO);
}

/*=====================================================================*/
//	process the thread requery
/*=====================================================================*/
void* thread_proc(void* info)
{
	MSG_PTR cli_info = info;

	switch(cli_info->cmd)
	{
		case CMD_SEND_ID:
		case CMD_SEND_ALL:
			send_msg(cli_info);
			break;

		case CMD_LOGIN:
			login(cli_info);
			break;

		case CMD_REGISTER:	
			chater_regist(cli_info);
			break;

		case CMD_LIST_CHATERS:
			list_chaters(cli_info);
			break;

		case CMD_JOIN_CHATROOM:
		case CMD_SEND_CHATROOM:
		case CMD_CREATE_CHATROOM:
		case CMD_LIST_CHATROOMS:
			chatroom_proc(cli_info);
			break;

		case CMD_CLIENT_EXIT:
			client_exit(cli_info);
			break;

		case CMD_HELP:
		case CMD_EXIT:
			break;
	}
	// return NULL;
}

int main(int argc, char** argv)
{
	// int fd_fifo;	// server fifo fd
	pthread_t p_cli;// client thread
	MSG client_msg;
	struct sigaction act;
	act.sa_handler = sign_kill;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESETHAND;
	sigaction(SIGKILL, &act, NULL);
	if(fork() > 0) exit(0);
	setsid();
	init(&ser_fd);

	while(1)
   	{
		bzero(&client_msg, sizeof(MSG));
		if(read(ser_fd, &client_msg, sizeof(MSG)) > 0)
		{
			pthread_create(&p_cli, NULL, thread_proc, &client_msg);
			pthread_join(p_cli, NULL);
		}
	}
	clear();
	return 0;
}
