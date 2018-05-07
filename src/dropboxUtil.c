#ifndef UTIL_CODE
#define UTIL_CODE

int ID_MSG_CLIENT = 0;

#include "dropboxUtil.h"

int contact_server(char *host, int port, UserInfo user) {

	//char buffer[BUFFER_SIZE];
	int func_return, sockid;
	unsigned int length;

	struct sockaddr_in serv_conn, from;
	
	Frame packet;
	
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockid == ERROR) {
		printf("Error opening socket ");
		return ERROR;
	}
	else
		printf("First client socket %i\n", sockid);

	bzero((char *) &serv_conn, sizeof(serv_conn));

	serv_conn.sin_family = AF_INET;
	serv_conn.sin_port = htons(port);
	serv_conn.sin_addr.s_addr = inet_addr(host);

	/* Filling packet structure */
	bzero(packet.user, MAXNAME-1);
	strcpy(packet.user, user.id);
	bzero(packet.buffer, BUFFER_SIZE -1);
	packet.message_id = ID_MSG_CLIENT;
	packet.ack = FALSE;

	while((strcmp(user.id, packet.user) != 0) || (packet.ack != TRUE) || (packet.message_id != ID_MSG_CLIENT)) {	

		printf("Entered\n");
		func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_conn, sizeof(struct sockaddr_in));
		if (func_return < 0) {
			printf("ERROR sendto ");
			return ERROR;
		}/*
		if (packet.ack == TRUE)
			printf("Ack 1 on client TRUE\n");
		else
			printf("Ack 1 on client FALSE\n");
		DEBUG*/
	
		length = sizeof(struct sockaddr_in);
		func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		if (func_return < 0) {
			printf("ERROR recvfrom ");
			return ERROR;
		}/*
		if (packet.ack == TRUE)
			printf("Ack 2 on client TRUE\n");
		else
			printf("Ack 2 on client FALSE\n");
		DEBUG*/

	}

	ID_MSG_CLIENT++;

	//printf("Got an ack: %s\n", packet.buffer); DEBUG

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

int getFileSize(FILE* file) {
	int size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	return size;
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

/**Tries to add a new device to a client, 
 * returns 1 if successfull
 * returns -1 if client reached the max amount of devices
 */
int newDevice(Client* client, int socket) {
	if(client->devices[0] == -1) {
		client->devices[0] = socket;
		return 0;
	}
	if(client->devices[1] == -1) {
		client->devices[1] = socket;
		return 0;
	}

	return -1;
}

int fileExists(char* filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
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



#endif
