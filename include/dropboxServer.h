#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "dropboxUtil.h"


/*typedef struct server_info {
  char ip[sizeof(DEFAULT_ADDRESS) * 2];
  char folder[MAXNAME * 2];
  int port;
}ServerInfo;

typedef struct client{
    int devices[2];
    char userid[MAXNAME];
    struct file_info files[MAXFILES];
    int logged_in;
}Client;

typedef struct connection_info{
  int socket_id;
  char* ip;
  char buffer[BUFFER_SIZE];
}Connection;

typedef struct client_node{
    Client* client;
    struct client_node* next;
    struct client_node* prev;   
}ClientNode;

typedef ClientNode* ClientList;*/


void sync_server();

void receive_file(char *filename, int sockid, int id);

void send_file(char *filename, int sockid, struct sockaddr_in *cli_addr);

#endif
