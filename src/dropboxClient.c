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
int ID_MSG_CLIENT = 0;
/////////////////////////

int login_server(char *host, int port) {
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
	
	user.socket_id = sockid;

	bzero((char *) &serv_conn, sizeof(serv_conn));

	serv_conn.sin_family = AF_INET;
	serv_conn.sin_port = htons(port);
	serv_conn.sin_addr.s_addr = inet_addr(host);
	user.serv_conn = &serv_conn;

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
		}
		
		length = sizeof(struct sockaddr_in);
		func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		if (func_return < 0) {
			printf("ERROR recvfrom ");
			return ERROR;
		}
	} ID_MSG_CLIENT++;
	serv_conn.sin_port = htons(atoi(packet.buffer)); //updates defauld port with port received from the server


	/* Sync the files from user to server */
	//sync_dir(sockid, user, serv_conn); -> NOT TESTED YET

	/*Cria sync_dir do usuário se não existir*/
	if(check_dir(user.folder) == FALSE) {
		if(mkdir(user.folder, 0777) != SUCCESS) {
			printf("Error creating server folder '%s'.\n", user.folder);
			return ERROR;
		}
	}

	return SUCCESS;
//-----------------------LOGIN SERVER DO VILMAR------------------------------------------------------------------
	/*struct sockaddr_in si_other;
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
		return SUCCESS;*/
}

void sync_client() {


}

void send_file(char *filename, int sockid, struct sockaddr_in *serv_conn) {
	char filepath[3*MAXNAME];
	int file_size;
	int bytes_sent;
	int func_return;
	sprintf(filepath, "%s/%s", user.folder, filename);

	Frame packet;
	bzero(packet.user, MAXNAME-1);
	strcpy(packet.user, user.id);
	bzero(packet.buffer, BUFFER_SIZE -1);
	packet.ack = FALSE;

	printf("\nRequisitando envio de arquivo para porta-%d/socket-%d", ntohs(serv_conn->sin_port), sockid); //DEBUG
	
	/* Sends upload request to server */
	strcpy(packet.buffer, UP_REQ);
	func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
	if (func_return < 0) {
		printf("ERROR sendto\n");
		return;
	}

	/* Receive ack from server */
	struct sockaddr_in from;
	unsigned int length = sizeof(struct sockaddr_in);
	func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
	if (func_return < 0) {
		printf("ERROR recvfrom\n");
		return;
	}

	if(packet.ack == FALSE) {
		printf("\nREQUEST TO UPLOAD NEGATED");
		return;
	}

	if(strcmp(packet.buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(packet.buffer, filename);
		printf("\nEnviando arquivo: %s\n", packet.buffer); //DEBUG
		func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
		if (func_return < 0) {
			printf("ERROR sendto\n");
			return;
		}
	}

	FILE* file;
	file = fopen(filepath, "rb");

	if(file) {
		file_size = getFileSize(file);
		if(file_size == 0) {
			fclose(file);
			printf("The file is empty\n");
			return;
		}
		
		sprintf(packet.buffer, "%d", file_size);
		//packet.ack == FALSE;
		/* Sends the file size to the server*/
		func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) serv_conn, sizeof(struct sockaddr));
		if (func_return < 0) 
			printf("ERROR on sendto\n");
	
		/* Sends the file in BUFFER_SIZE sized parts*/
		bytes_sent = 0;
		while(!feof(file)) {
			fread(packet.buffer, sizeof(char), BUFFER_SIZE, file);
			bytes_sent += sizeof(char) * BUFFER_SIZE;

			func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) serv_conn, sizeof(struct sockaddr));
			if (func_return < 0) 
				printf("ERROR on sendto\n");

			printf("\n Sending file %s - Total: %d / Read: %d", filepath, file_size, bytes_sent); //DEBUG
		}
		printf("\n Finished sending file %s\n", filepath);
		fclose(file);
	}
	else
		printf("\nErro ao abrir o arquivo %s\n", filepath);
}

void get_file(char *file) {


}

void delete_file(char *file) {


}

void close_session() {


}

void client_menu() {
	char command_line[MAXPATH];
	char *command;
	char *attribute;
	
	int exited = FALSE;
	while(!exited){
		printf("\nDigite um comando:");

		if(fgets(command_line, sizeof(command_line)-1, stdin) != NULL) {
			command_line[strcspn(command_line, "\r\n")] = 0;

			if (strcmp(command_line, "exit") == 0) 
				exited = TRUE;
			else {
				command = strtok(command_line, " ");
				attribute = strtok(NULL, " ");
			}
			//printf("\necho: %s %s", command, attribute); //DEBUG

			/* UPLOAD */
			if(strcmp(command, "upload") == 0) {
				send_file(attribute, user.socket_id, user.serv_conn);
			}

		}
		else
			printf("\nComando invalido");
	}

	close_session();
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
	sockid = login_server(address, port); 
	if (sockid == SUCCESS) {
		/*Cria sync_dir do usuário se não existir*/
		sprintf(user.folder, "%s/sync_dir_%s", getUserHome(),user.id);
		if(check_dir(user.folder) == FALSE) {
			if(mkdir(user.folder, 0777) != SUCCESS) {
				printf("Error creating server folder '%s'.\n", user.folder);
				return ERROR;
			}
		}
		client_menu();

	} else {
		printf("Could not connect to server '%s' at port '%d'\n", address, port);
		return ERROR;
	}
}


#endif
