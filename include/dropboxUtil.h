#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h> 
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_PORT 3000
#define DEFAULT_ADDRESS "127.0.0.1"
#define SERVER_FOLDER "syncBox_users"

#define MAXNAME 25
#define MAXFILES 50
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE !(TRUE)


typedef struct file_info{
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
}FileInfo;

typedef struct server_info {
	char ip[sizeof(DEFAULT_ADDRESS) * 2];
	char folder[MAXNAME * 2];
	int port;
}ServerInfo;

typedef struct user_info {
	char id[MAXNAME];
	char folder[MAXNAME*2];
}UserInfo;

typedef struct connection_info{
	int socket_id;
	char* ip;
	char buffer[BUFFER_SIZE];
}Connection;

typedef struct client{
	int devices[2];
	char userid[MAXNAME];
	int logged_in;
	struct file_info files[MAXFILES];
}Client;

typedef struct client_node{
	Client* client;
	struct client_node* next;
	struct client_node* prev;   
}ClientNode;

typedef ClientNode* ClientList;


/* Ack Structures */
typedef struct packet{
    char data[BUFFER_SIZE];
}Packet;

typedef struct frame{
    int message_id;
    bool ack;
    Packet packet;
}Frame;
/* End of Ack */

int contact_server(char *host, int port, UserInfo user);

void sync_dir();

char* getUserHome();

//int open_server(char *address, int port, ServerInfo serverInfo);

bool check_dir(char *pathname);

Client* searchClient(char* userId, ClientList user_list);

ClientList addClient(char* userID, int socket, ClientList user_list);


//DEBUG SECTION
void printUserList(ClientList user_list);

#endif
