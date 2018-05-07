#ifndef CLIENT_CODE
#define CLIENT_CODE

#include "dropboxClient.h"

/*   Global variables   */
UserInfo user;
/////////////////////////

int login_server(char *host, int port) {

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
