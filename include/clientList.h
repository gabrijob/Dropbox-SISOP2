#ifndef CLIENTLIST_HEADER
#define CLIENTLIST_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dropboxServer.h"

typedef struct client_node{
    Client* client;
    struct client_node* next;
    struct client_node* prev;   //eventualmente ver se precisa ser duplamente encadeada mesmo
}ClientNode;

typedef ClientNode* ClientList;



ClientList addClient(char* userID, int socket, ClientList user_list);

void printUserList(ClientList user_list);


#endif