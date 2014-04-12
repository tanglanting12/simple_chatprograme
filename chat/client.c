/**
 * fifo client 
 * author jayhoo
 */
#include "chat.h"

/*===============================================================*/
/* fifo_name: client fifo name, 
 * cur_name: current login client name
 * msg is: client sent message to server
 * response: server sent message to client 
 * join_info: update by the thread, if the client join chatroom success or not
 * all that see chat.h
 */
/*===============================================================*/
char fifo_name[50];
char cur_name[50];
char join_info[50];


/*===============================================================*/
// pre declaration
/*===============================================================*/
void init(int* cli_fd, int* ser_fd);
void process(int cli_fd, int ser_fd);    		// process first use the client
void chat(int cli_fd, int ser_fd);        		// start chat
void list_chaters(int cli_fd, int ser_fd);		// list all online chaters
void send_to_all(int cli_fd, int ser_fd); 		// send to all the online chaters
void send_to_chater(int cli_fd, int ser_fd);	// send to target chater
void list_chatrooms(int cli_fd, int ser_fd);	// list the chatrooms
void join_chatrooms(int cli_fd, int ser_fd); 	// join a chatroom
void create_chatrooms(int cli_fd, int ser_fd);	// create a chatroom
void cli_exit(int ser_fd);						// send to server if client exit
void* thread_proc(void* info);					// thread process

/*===============================================================*/
// first use clint need to init resource
// cli_fd: client fifo fd
// ser_fd: server fifo fd
/*===============================================================*/
void init(int* cli_fd, int* ser_fd)
{
	bzero(fifo_name, strlen(fifo_name));
	sprintf(fifo_name, CLIENT_FIFO, getpid());
	// build client fifo
	CHK(0, mkfifo(fifo_name, 0777), "can not create client fifo");
	CHK(0, access(fifo_name, F_OK), "can not access client fifo");
	CHK_E(-1, *cli_fd = open(fifo_name, O_RDWR), "can not open client fifo");
	CHK(0, access(SERVER_FIFO, R_OK | W_OK), "can not read or write server fifo");
	CHK_E(-1, *ser_fd = open(SERVER_FIFO, O_WRONLY | O_NONBLOCK), "can not open server fifo for writting");
}

/*===============================================================*/
// process method process loin and register
/*===============================================================*/
void process(int cli_fd, int ser_fd)
{
	int choice;
	MSG msg;
	RESPONSE response;
	bzero(&response, sizeof(RESPONSE));

	while(1)
	{
		printf("please chose:\n1.login 2.register 3.exit\n");
		scanf("%d%*c", &choice);
		switch(choice)
		{
			case 1:
				msg.cmd = CMD_LOGIN;
				break;
			case 2:
				msg.cmd = CMD_REGISTER;
				break;
			case 3:
			default:
				printf("exit or unknow command!\n");
				return;
		}
		// write to the ser_fifo for the first time if want to regitster or login
		// must loin can see the command like "send to all" or "send to **" or "join a group"
		memset(msg.send_name, '\0', strlen(msg.send_name));
		memset(msg.pwd, '\0', strlen(msg.send_name));
		printf("please input you name:  ");
		scanf("%s%*c", msg.send_name);
		printf("please input you passwd: ");
		scanf("%s%*c", msg.pwd);
		msg.pid = getpid();
		// strncpy(msg.send_name, msg.msg, strlen(msg.msg));
		CHK_E(-1, write(ser_fd, &msg, sizeof(msg)), "can not write the server fifo");
		read(cli_fd, &response, sizeof(RESPONSE));
		if(response.login_s_f == LOGIN_SUCCESS)
		{
			// login success
			// save current name
			bzero(cur_name, strlen(cur_name));
			strncpy(cur_name, msg.send_name, strlen(msg.send_name));
			break;
		} 
		else if(!strcmp(response.msg, LOGIN_PWD_ERROR))
		{
			// pwd error
			printf("check you pwd\n");
		}
		else if(!strcmp(response.msg, LOGIN_NO_SUCH_CHATER))
		{
			// default no such user
			printf("no such user! please register again\n");
		}
		else if(!strcmp(response.msg, REGISTER_SUCCESS))
		{
			printf("register successful, please login\n");
		}
		else if(!strcmp(response.msg, REGISTER_FAIL))
		{
			printf("register fail, please try again\n");
		}
		bzero(&response.msg, strlen(response.msg));
	} // while
	chat(cli_fd, ser_fd);
}

/*==================================================================*/
// process start chat
// client login enable, process the client to chat
/*==================================================================*/
void chat(int cli_fd, int ser_fd)
{
	int choice;
   	pthread_t t_id;
	RESPONSE response;
	MSG msg;
	bzero(&response, sizeof(response));
	pthread_create(&t_id, NULL, thread_proc, (void*)&cli_fd);
	while(1)
	{
		bzero(&msg, sizeof(MSG));
		printf("please chose:\n1.list online chater 2.send to all online chater 3.sent to user\n");
		printf("4.list all chatrooms  5.join chatrooms 6.crate chatroom 7.exit\n>");
		fflush(stdout);
		scanf("%d%*c", &choice);
		switch(choice)
		{
			case 1:
				// there may be some problem 
				// if there are a lot's of online chater
				list_chaters(cli_fd, ser_fd);
				break;
			case 2:
				send_to_all(cli_fd, ser_fd);
				break;
			case 3:
				send_to_chater(cli_fd, ser_fd);
				break;
			case 4:
				list_chatrooms(cli_fd, ser_fd);
				break;
			case 5:
				join_chatrooms(cli_fd, ser_fd);
				break; 
			case 6:
				create_chatrooms(cli_fd, ser_fd);
				break;
			case 7:
				cli_exit(ser_fd);
				pthread_cancel(t_id);
				pthread_join(t_id, NULL);
				printf("client exit!\n");
				return;
			default:
				printf("please input right choice\n");
				break;
		} // switch
	} // while
}

/*=================================================================*/
// client list all online chaters in server
// the message respone by server
/*=================================================================*/
void list_chaters(int cli_fd, int ser_fd)
{
	int _i, _j;
	MSG msg;
	bzero(&msg, sizeof(MSG));
	printf("\e[33mplese wait for server response...\e[0m\n");
	msg.pid = getpid();
	msg.cmd = CMD_LIST_CHATERS;			// list chaters command
	write(ser_fd, &msg, sizeof(msg));	// write to server
}

/*===============================================================*/
// sent to all online chaters
// default not loop, if loop it will make the system more busier
/*================================================================*/
void send_to_all(int cli_fd, int ser_fd)
{
	MSG msg;
	bzero(&msg, sizeof(MSG));
	printf("please input you message(less than 50): \n>");
	fflush(stdout);
	msg.cmd = CMD_SEND_ALL;
	strncpy(msg.send_name, cur_name, strlen(cur_name));
	// %*c ignore the '\n'
	scanf("%s%*c", msg.msg);
	msg.pid = getpid();
	if(write(ser_fd, &msg, sizeof(msg)) == -1) 
	{
		printf("[sent to all online chaters fail, please try again]\n");
		return;
	}
	printf("[sent to all online chaters success]\n");
}

/*========================================================*/
// sent to target chater
// loop if the chater use '-quit' to exit
// example-> xx(chater's name):nnnn(message)
/*========================================================*/
void send_to_chater(int cli_fd, int ser_fd)
{
	int _i, _j;
	char _msg[50], *name;
	MSG msg;
	printf("\e[32mplease read the message:\n");
	printf("First: input you target chater's name and use ':' to separate the message and name\n");
	printf("Second: chat as long as you want, use '-quit' to quit\n");
	printf("Third: also can use '-help' to gain help infomation\n");
	printf("Forth: message must not long than 50 charcaters\e[0m\n");
	while(1)
	{
		printf(">");
		fflush(stdout);
		bzero(_msg, strlen(_msg));
		bzero(&msg, sizeof(MSG));
		scanf("%s%*c", _msg);
		if(!strcmp(_msg, "-quit")) 
		{
			cli_exit(ser_fd);
			return;
		}
		else if(!strcmp(_msg, "-help"))
		{
			printf("\e[33m[Message Example-> xx:i want to say to you\n '-quit' to exit]\e[0m\n");
		}
		else 
		{
			// default process the message
			// TODO strtok has some problem
			// name = strtok(_msg, ":");
			
			for(_j = _i = 0; _i < strlen(_msg); ++_i)
			{
				if(_msg[_i] == ':') _j = _i;
			}
			if(_j) // can find the ':'
			{
				// separate the input of the chater
				msg.cmd = CMD_SEND_ID;
				msg.pid = getpid();
				strncpy(msg.send_name, cur_name, strlen(cur_name));
				strncpy(msg.recv_name, _msg, _j);
				// strncpy(msg.msg, _msg, strlen(_msg));
				for(_i = 0; _j < strlen(_msg); )
				{
					msg.msg[_i++] = _msg[++_j];
				}
				// write to the server, to tranform the informations
				if(-1 !=  write(ser_fd, &msg, sizeof(msg)))
				{						
					printf("[send success]\n");
					continue;
				}
				printf("[send fail, please try again]\n");
			}
			else 
			{
				printf("\e[33m[please input the right pattern of the message, use '-help' to get more infomations]\e[0m\n");
			}
		} // else 
	} // while
}

/*========================================================*/
// list chatrooms
// to enable the chater join chatrooms
/*========================================================*/
void list_chatrooms(int cli_fd, int ser_fd)
{
	MSG msg;
	bzero(&msg, sizeof(MSG));
	msg.pid = getpid();
	msg.cmd = CMD_LIST_CHATROOMS;
	strncpy(msg.send_name, cur_name, strlen(cur_name));
	CHK_R(-1, write(ser_fd, &msg, sizeof(msg)), "join chatroom-can not join the chatroom");
}

/*========================================================*/
// join chatrooms
/*========================================================*/
void join_chatrooms(int cli_fd, int ser_fd)
{
	char _msg[50] = {'\0'};
	int flag = 0;
	MSG msg;
	bzero(&msg, sizeof(MSG));
	msg.cmd = CMD_JOIN_CHATROOM;
	msg.pid = getpid();
	strncpy(msg.send_name, cur_name, strlen(cur_name));
	printf("[please input you chatroom' name, user '-quit' to exit]\n>");
	scanf("%s%*c", _msg);
	strncpy(msg.msg, _msg, strlen(_msg));
	write(ser_fd, &msg, sizeof(MSG));
	while(1)
	{
		if(strcmp(join_info, "join success") && !flag)  
		{
			bzero(msg.msg, strlen(msg.msg));
			printf("[please input you chatroom' name, user '-quit' to exit]\n>");
			strncpy(msg.recv_name, _msg, strlen(_msg));
			bzero(_msg, strlen(_msg));
			scanf("%s%*c", _msg);
			if(!strcmp(join_info, "join success")) 
			{
				flag = 1;
				// bzero(join_info, strlen(join_info));
				msg.cmd = CMD_SEND_CHATROOM;
			}
			else 
			{
				if(!strcmp(_msg, "-quit")) return;
				strncpy(msg.msg, _msg, strlen(_msg));
				write(ser_fd, &msg, sizeof(MSG));
				continue;
			}
		}
		else if(flag)
		{
			// printf("[%s]", msg.recv_name);
			bzero(msg.msg, strlen(msg.msg));
			printf(">");
			scanf("%s%*c", _msg);
			msg.cmd = CMD_SEND_CHATROOM;
				flag = 1;
		}

		if(_msg[0] != '-') 
		{
			// command is start with '-'
			strncpy(msg.msg, _msg, strlen(_msg));
		}
		else if(!strcmp(_msg, "-quit"))
		{
			printf("[client quit the task]\n");
			bzero(join_info, strlen(join_info));
			// cli_exit(ser_fd);
			return;
		}
		else if(!strcmp(_msg, "-help"))
		{
			printf("\e[33mInput any message less than 50 chars, use '-quit' to quit\e[0m\n");
			continue;
		}
		write(ser_fd, &msg, sizeof(msg));
		fflush(stdout);
	}
}

/*========================================================*/
// create chatrooms
// allow the current to create the chatroom
// default: the chater join the new create chatroom
/*========================================================*/
void create_chatrooms(int cli_fd, int ser_fd)
{
	MSG msg;
	bzero(&msg, sizeof(MSG));
	strncpy(msg.send_name, cur_name, strlen(cur_name));
	msg.cmd = CMD_CREATE_CHATROOM;
	msg.pid = getpid();
	printf("[Please input a chatroom name(less than 30 chars)]:");
	scanf("%s%*c", msg.msg);
	CHK_R(-1, write(ser_fd, &msg, sizeof(msg)), "create chatroom-can not create");
}

/*========================================================*/
// process thread: simply to read the fifo
/*========================================================*/
void* thread_proc(void* info)
{
	// info: cli_fd;
	MSG _msg;
	int cli_fd = *((int*)info), _i, _j;
	while(1)
	{
		bzero(&_msg, sizeof(MSG));
		if(read(cli_fd, &_msg, sizeof(MSG)) > 0)
		{
			// save the info for function join_chatroom
			if(strcmp(join_info, "join success"))
			{
				bzero(join_info, strlen(join_info));
				strncpy(join_info, _msg.msg, strlen(_msg.msg));
			}
			// start with '*' is to list the chaters
			if(_msg.msg[0] == '*')
			{
				char ch[50] = { '\0' };
				for(_j = 0, _i = 1; _i < strlen(_msg.msg); )
				{
					// separate the chater name by "*"
					if((ch[_j++] = _msg.msg[_i++]) == '*')
					{
						ch[--_j] = '\0';
						printf("\e[33m[ %s ] \e[0m", ch);
						bzero(ch, strlen(ch));
						_j = 0;
					}	
				}
				printf("\n>");
			} // if
			else 
			{
				printf("\e[34m %s:%s \e[0m\n>", _msg.send_name, _msg.msg);
			}
			fflush(stdout);
		}
	}
	return NULL;
}
/*========================================================*/
// client exit
// if client exit send a message to info the server
/*========================================================*/
void cli_exit(int ser_fd)
{
	MSG msg;
	bzero(&msg, sizeof(MSG));
	msg.pid = getpid();
	strncpy(msg.send_name, cur_name, strlen(cur_name));
	msg.cmd = CMD_CLIENT_EXIT;
	CHK_R(-1, write(ser_fd, &msg, sizeof(msg)), "client exit info can't write to server");
}

/*========================================================*/
// main function
/*========================================================*/
int main(int argc, char** argv)
{
	int cli_fifo, ser_fifo;
	init(&cli_fifo, &ser_fifo);
	process(cli_fifo, ser_fifo);
	close(cli_fifo);
	close(ser_fifo);
	unlink(fifo_name);
	return 0;
}
