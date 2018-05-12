#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h> 
#include <stdbool.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

#define DYN_PORT_START 49153
#define DYN_PORT_END 65535
#define DEFAULT_PORT 3000
#define DEFAULT_ADDRESS "127.0.0.1"
#define SERVER_FOLDER "syncBox_users"
#define SERVER_USER "server"

#define MAXNAME 25
#define MAXFILES 50
#define MAXPATH MAXFILES*MAXNAME
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE !(TRUE)

#define END_REQ "END SESSION REQUEST"
#define UP_REQ "FILE UPLOAD REQUEST"
#define F_NAME_REQ "FILE NAME REQUEST"

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
	char folder[MAXNAME * 2];
}UserInfo;

typedef struct connection_info{
	int socket_id;
	char client_id[MAXNAME];
	char* ip;
	char buffer[BUFFER_SIZE];
	int port;
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


/* Ack Structure */
/*
	->message_id	:	contains the id of the message, must be incremented every new message
	->ack		:	if the ack is confirmed must be TRUE
	->buffer	:	contains the string content of the message
	->user		:	contains the info of who sent the message (always as client, server user is default)
*/
typedef struct frame{
    int message_id;			
    bool ack;				
    char buffer[BUFFER_SIZE];
    char user[MAXNAME];
}Frame;
/* End of Ack */

int contact_server(char *host, int port, UserInfo user);

void sync_dir(int sockid, UserInfo user, struct sockaddr_in cli_addr);
void sync_server(int sock_s, Client *client_s);

int get_dir_file_info(char * path, FileInfo files[]);
void getFileExtension(const char *filename, char* extension);
void *dir_content_thread(void *ptr);
//int get_dir_content(char *path, struct d_file files[], int* counter);

void getModifiedTime(char *path, char *last_modified);
time_t getTime(char *last_modified);
int older_file(char *last_modified, char *aux);
int newDevice(Client* client, int socket);
int fileExists(char* filename);
int getFileSize(FILE* file);
char* getUserHome();
bool check_dir(char *pathname);

Client* searchClient(char* userId, ClientList user_list);
ClientList addClient(char* userID, int socket, ClientList user_list);

//DEBUG SECTION
void printUserList(ClientList user_list);

#endif
