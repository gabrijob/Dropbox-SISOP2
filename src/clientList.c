#ifndef CLIENTLIST_CODE
#define CLIENTLIST_CODE

#include "clientList.h"



ClientList addClient(char* userID, int socket, ClientList user_list){
    Client* new_client = (Client*) malloc(sizeof(Client));

    strcpy(new_client->userid, userID);
    new_client->devices[0] = socket;
    new_client->devices[1] = -1;
    //newclient->files = getFilesFromSyncDir ou alguma coisa parecida
    new_client->logged_in = 1;

    ClientNode* new_node = (ClientNode*) malloc(sizeof(ClientNode));
    new_node->client = new_client;

    if(user_list == NULL){
        user_list = new_node;
        new_node->next = NULL;
        new_node->prev = NULL;
    }
    else{
        ClientNode* current_node = user_list;

        while(current_node->next != NULL){
            current_node = current_node->next;
        }

        current_node->next = new_node;
        new_node->prev = current_node;
    }

    return user_list;
}

void printUserList(ClientList user_list){
    ClientNode* current_node = user_list;

    
    while(current_node != NULL){
        printf("\n User: %s | Devices:", current_node->client->userid);
        if(current_node->client->devices[0] > 0)
            printf(" socket-%i ", current_node->client->devices[0]);
        if(current_node->client->devices[1] > 0)
            printf(" socket-%i ", current_node->client->devices[1]);
        printf("\n");
        current_node = current_node->next;
    };
}

#endif