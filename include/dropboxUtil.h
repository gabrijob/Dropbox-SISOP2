#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h> 
#include <stdbool.h>
#include <time.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>

#include "udp_assist.h"


#define DYN_PORT_START 49153
#define DYN_PORT_END 65535
#define DEFAULT_PORT 3000
#define DEFAULT_ADDRESS "127.0.0.1"
#define SERVER_FOLDER "syncBox_users"
#define SERVER_USER "server"

#define MAXDEVICES 2
#define MAXNAME 25
#define MAXFILES 50
#define MAXPATH MAXFILES*MAXNAME
#define MAX_CLIENTS 10

#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE !(TRUE)

/* Communication constants */
#define START_MSG_COUNTER 0
#define CLI_COM_TYPE "CLIENT COMMUNICATION"
#define SVR_COM_TYPE "SERVER COMMUNICATION"
#define DEFAULT_SERVER "--primary"
#define BACKUP_SERVER "--backup"

#define END_REQ "END SESSION REQUEST"
#define UP_REQ "FILE UPLOAD REQUEST"
#define UP_REQ_S "FILE UPLOAD REQUEST SERVER ONLY"
#define F_NAME_REQ "FILE NAME REQUEST"
#define F_NAME_NREQ "FILE NAME NOT REQUIRED"
#define DOWN_REQ "FILE DOWNLOAD REQUEST"
#define LIST_S_REQ "LIST SERVER FILES REQUEST"
#define DEL_REQ "FILE DELETE REQUEST"
#define DEL_REQ_S "FILE DELETE REQUEST SERVER ONLY"
#define DEL_COMPLETE "FILE DELETED"
#define SYNC_REQ "SYNCHRONIZATION REQUEST"
#define SYNC_NREQ "SYNCHRONIZATION NOT REQUIRED"
#define NEED_SYNC "IS SYNCHRONIZATION NEEDED"

#define OK "OK"
#define NOT_OK "NOT_OK"

#define S_SYNC "sync"
#define S_GET "get"
#define S_OK "ok"

typedef struct file_info{
	char name[MAXNAME];
	char extension[MAXNAME];
	char last_modified[MAXNAME];
	int size;
}FileInfo;

typedef struct msg_id{
	int server;
	int client;
}MSG_ID;

typedef struct server_info {
	char ip[sizeof(DEFAULT_ADDRESS) * 2];
	char folder[MAXNAME * 2];
	int port;
}ServerInfo;

typedef struct user_info {
	char id[MAXNAME];
	char folder[MAXNAME * 2];
	int socket_id;
	struct sockaddr_in *serv_conn;
	pthread_mutex_t lock_server_comm;
	struct msg_id *msg_id;
}UserInfo;

typedef struct connection_info{
	int socket_id;
	char client_id[MAXNAME];
	char* ip;
	char buffer[BUFFER_SIZE];
	int port;
	struct sockaddr_in *client_address;
}Connection;

typedef struct client{
	int devices[MAXDEVICES];
	char userid[MAXNAME];
	int logged_in;
	struct file_info files[MAXFILES];
	pthread_mutex_t mutex_files[MAXFILES];
	int n_files;
	int pending_changes[2];
}Client;

typedef struct client_node{
	Client* client;
	struct client_node* next;
	struct client_node* prev;   
}ClientNode;

typedef ClientNode* ClientList;

typedef struct d_file {
  char path[3*MAXNAME];
  char name[MAXNAME];
} DFile;

typedef struct dir_content {
  char* path;
  struct d_file* files;
  int* counter;
} DirContent;


int new_server_port(char *address, int *socket_id, int *port);
int getFileIndex(char *filename, FileInfo files[], int number_of_files);
int get_dir_file_info(char * path, FileInfo files[]);
void getFileExtension(const char *filename, char* extension);
void *dir_content_thread(void *ptr);
int get_dir_content(char *path, struct d_file files[], int* counter);
int print_dir_file_info(char * path);

void getModifiedTime(char *path, char *last_modified);
time_t getTime(char *last_modified);
int older_file(char *last_modified, char *aux);
int newDevice(Client* client, int socket);
int removeDevice(Client* client, int device);
int getDevice(Client* client, int socket);
int devicesOn(Client* client);
int fileExists(char* filename);
int getFilesize(FILE* file);
int getFileSize(char *path);
char* getUserHome();
bool check_dir(char *pathname);

Client* searchClient(char* userId, ClientList user_list);
ClientList addClient(char* userID, int socket, ClientList user_list);
ClientList check_login_status(Client* client, ClientList client_list);
ClientList removeClient(Client* client, ClientList client_list);

//DEBUG SECTION
void printUserList(ClientList user_list);
void printClientFiles(Client* client);

#endif
