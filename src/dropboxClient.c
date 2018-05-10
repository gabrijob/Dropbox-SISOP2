#ifndef CLIENT_CODE
#define CLIENT_CODE
#define SUCCESS 0
#define ERROR -1

#include "dropboxClient.h"
#include "sys/socket.h" 
#include "arpa/inet.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"


/*   Global variables   */
UserInfo user;
/////////////////////////

int login_server(char *host, int port) {
	struct sockaddr_in si_other;
	char buffer[BUFFER_SIZE]
	int sockid, i, socketid_len = sizeof(si_other);

//creating socket
	if ((sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
		printf("Error when creating socket\n");
		return ERROR;
	}

//zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);

//bind socket to port
	if (bind(sockid, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
		printf("Error when binding socket\n");
		return ERROR;
	}

	if((inet_aton(SERVER, &si_other.sin_addr)) == 0){
		printf("Error in inet_aton()\n");
	}

//zero out the buffer
	bzero(buffer, BUFFER_SIZE-1);

	strcpy(buffer, user.id);

//send the id from user to server
	if((sendto(sockid, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &si_other, slen)) == -1){
		printf("Error in sento()\n");
		return ERROR;
	}

//receive a reply and print it
//clear the buffer by filling null, it might have previously received data
	memset(buffer, '\0', BUFFER_SIZE);

//try to receive the id from server, blocking call
	if((recvfrom(sockid, buffer, 0, (struct sockaddr*) &si_other, slen)) == -1){
		printf("Error in recvfrom()\n");
		return ERROR;
	}
	else
		printf("Success in login_server()\n");
		return SUCCESS;
}

void sync_client() {


}

void send_file(char *file) {


}

void get_file(char *file) {


}

void delete_file(char *file) {


}

void close_session() {


}

int main(int argc, char *argv[]) {

	
	int port, sockid;
	char *address;


	if (argc != 4) {
		puts("Error! Insuficient Arguments");
		puts("Expected: './dropboxClient user address port'");

		return ERROR;
	}

	/* Setting user information by parsing entries */
	if (strlen(argv[1]) <= MAXNAME) {
		if (strcmp(argv[1], SERVER_USER) == 0) {
			printf("Invalid User! Please do not use 'server' as ID...\n");
			return ERROR;
		}

		strcpy(user.id, argv[1]);
		sprintf(user.folder, "%s/sync_dir_%s", getUserHome(), user.id);
	} else {
		puts("Maximum user ID size exceeded\n");
		printf("Maximum: %d\n", MAXNAME);

		return ERROR;
	}
	
	address = malloc(strlen(argv[2]));
	strcpy(address, argv[2]);

	port = atoi(argv[3]);
	/* End of initial parsing */


	/* Starts communication with the server
	        -> Opens a socket UDP */
	sockid = contact_server(address, port, user); 
	if (sockid == SUCCESS) {

		printf("Ready to show menu\n");

	} else {
		printf("Could not connect to server '%s' at port '%d'\n", address, port);
		return ERROR;
	}
}


#endif
