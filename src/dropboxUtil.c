#ifndef UTIL_CODE
#define UTIL_CODE

#include "dropboxUtil.h"

int contact_server(char *host, int port, UserInfo user) {

	char buffer[BUFFER_SIZE];
	int func_return, sockid;
	unsigned int length;

	struct sockaddr_in serv_conn, from;
	
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockid == ERROR) {
		printf("Error opening socket ");
		return ERROR;
	}

	bzero((char *) &serv_conn, sizeof(serv_conn));

	serv_conn.sin_family = AF_INET;
	serv_conn.sin_port = htons(port);
	serv_conn.sin_addr.s_addr = inet_addr(host);

	bzero(buffer, BUFFER_SIZE-1);
	strcpy(buffer, user.id);

	func_return = sendto(sockid, buffer, BUFFER_SIZE, 0, (const struct sockaddr *) &serv_conn, sizeof(struct sockaddr_in));
	if (func_return < 0) {
		printf("ERROR sendto ");
		return ERROR;
	}
	
	length = sizeof(struct sockaddr_in);
	func_return = recvfrom(sockid, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &from, &length);
	if (func_return < 0) {
		printf("ERROR recvfrom ");
		return ERROR;
	}

	printf("Got an ack: %s\n", buffer);

	return SUCCESS;

}


/* Used to sync directories */
void sync_dir() {




}


/*
	getpwuid() --> Gets users initial working directory
	geteuid()  --> Gets the effective user ID of the current process
*/

char* getUserHome() {
 	struct passwd *pw = getpwuid(geteuid());	 

	if (pw) {
    		return pw->pw_dir;
  	}
	
	printf("No root directory found! Returning empty...\n");
 	return "";
}


/* Verfies if the server directory already exists 
   	->Uses the 'sys/stat.h' lib */

bool check_dir(char *pathname) {
	struct stat st = {0};
	
	if (stat(pathname, &st) == ERROR)
		return FALSE;
	
	return TRUE;
}


/* Seraches for client with the userId passed as an arument in the GLOBAL clients list
	-> returns the client's node if found or NULL if not */

Client* searchClient(char* userId, ClientList user_list) {

	ClientList current = user_list;

	while(current != NULL) {
		if(strcmp(userId, current->client->userid) == 0 && current->client->logged_in == 1) {
			return current->client;
		}
		current = current->next;
	}
	return NULL;
}



/* Adds a new client with the userId passed as an arument in the GLOBAL clients list
	-> returns the updated clients list */

ClientList addClient(char* userID, int socket, ClientList user_list) {

	Client* new_client = (Client*) malloc(sizeof(Client));

	strcpy(new_client->userid, userID);
	new_client->devices[0] = socket;
	new_client->devices[1] = -1;
	new_client->logged_in = 1;

	ClientNode* new_node = (ClientNode*) malloc(sizeof(ClientNode));
	new_node->client = new_client;

	if(user_list == NULL) {
		user_list = new_node;
		new_node->next = NULL;
		new_node->prev = NULL;
	} else {
	ClientNode* current_node = user_list;

	while(current_node->next != NULL) {
	    current_node = current_node->next;
	}

	current_node->next = new_node;
	new_node->prev = current_node;

	}

	return user_list;
}


/* --------------------------> FOR DEBUG <-------------------------- */
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
    }
}
/* ----------------------------------------------------------------- */


/*Verfies if the client can connect another devide; Returns -1 if not 
int addDevice(Client* client, int socket) {
	if(client->devices[0] == -1) {
		sprintf(client_folder, "%s/%s", serverInfo.folder, client->userid);
		client->n_files = get_dir_file_info(client_folder, client->file_info);
		client->devices[0] = socket;

		return 0;
	}

	if(client->devices[1] == -1) {
		sprintf(client_folder, "%s/%s", serverInfo.folder, client->userid);
		client->n_files = get_dir_file_info(client_folder, client->file_info);
		client->devices[1] = socket;

		return 1;
	}

	return -1;
}
*/


#endif
