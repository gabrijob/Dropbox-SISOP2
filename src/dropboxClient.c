#ifndef CLIENT_CODE
#define CLIENT_CODE

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
	sync_client(sockid, user, serv_conn); //-> NOT TESTED YET

	/*Cria sync_dir do usuário se não existir*/
	if(check_dir(user.folder) == FALSE) {
		if(mkdir(user.folder, 0777) != SUCCESS) {
			printf("Error creating server folder '%s'.\n", user.folder);
			return ERROR;
		}
	}

	return SUCCESS;
}



void send_file_client(char *filename) {
	char filepath[3*MAXNAME];
	int file_size;
	int bytes_sent;
	int func_return;
	sprintf(filepath, "%s/%s", user.folder, filename);

	int sockid = user.socket_id;
	struct sockaddr_in* serv_conn = user.serv_conn;

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
		file_size = getFilesize(file);
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

void get_file(char *filename) {
	int file_size;
	int bytes_received;
	int func_return;

	int sockid = user.socket_id;
	struct sockaddr_in* serv_conn = user.serv_conn;

	Frame packet;
	bzero(packet.user, MAXNAME-1);
	strcpy(packet.user, user.id);
	bzero(packet.buffer, BUFFER_SIZE -1);
	packet.ack = FALSE;


	/* Sends download request to server */
	strcpy(packet.buffer, DOWN_REQ);
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
		printf("\nREQUEST TO DOWNLOAD NEGATED");
		return;
	}

	if(strcmp(packet.buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(packet.buffer, filename);
		printf("\nPedindo arquivo: %s\n", packet.buffer); //DEBUG
		func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
		if (func_return < 0) {
			printf("ERROR sendto\n");
			return;
		}
	}

	char filepath[3*MAXNAME];
	sprintf(filepath, "%s/%s", user.folder, filename);
	printf("Receiving file at %s", filepath); //DEBUG
	FILE* file;
	file = fopen(filepath, "wb");

	if(file) {
		/* Receives the file size from server */
		func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		if (func_return < 0) 
			printf("ERROR recvfrom\n");

		file_size = atoi(packet.buffer);
		
		bzero(packet.buffer, BUFFER_SIZE -1);		
		packet.ack = TRUE;

		bytes_received = 0;
		while(file_size > bytes_received) {
			func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
			if (func_return < 0) 
				printf("ERROR recvfrom\n");

			if((file_size - bytes_received) > BUFFER_SIZE) {
				fwrite(packet.buffer, sizeof(char), BUFFER_SIZE, file);
				bytes_received += sizeof(char) * BUFFER_SIZE; 
			}
			else {
				fwrite(packet.buffer, sizeof(char), (file_size - bytes_received), file);
				bytes_received += sizeof(char) * (file_size - bytes_received);
			}
			printf("\n Receiving file %s - Total: %d / Written: %d", filename, file_size, bytes_received); //DEBUG
		}
		printf("\n Finished receiving file %s", filename); //DEBUG
		fclose(file);
	}
	else
		printf("\nErro ao abrir o arquivo %s", filepath);
}

void delete_file(char *file) {
/*
	Frame packet;
	bzero(packet.user, MAXNAME-1);
	strcpy(packet.user, user.id);
	bzero(packet.buffer, BUFFER_SIZE -1);
	packet.ack = FALSE;
	int sockid;

	char filename[MAXNAME];

	//send delete request to server
	strcpy(packet.buffer, DEL_REQ);
	func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
	if (func_return < 0) {
		printf("ERROR sendto DEL_REQ\n");
	}

	//Receive ack from server 
	struct sockaddr_in from;
	unsigned int length = sizeof(struct sockaddr_in);
	func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
	if (func_return < 0) {
		printf("ERROR recvfrom from delete\n");
	}

	if(packet.ack == FALSE) {
		printf("\nREQUEST TO DELETE NEGATED");
	}

	//Mandando o nome do arquivo pro servidor
	if(strcmp(packet.buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(packet.buffer, filename);
		printf("Deletando arquivo: %s\n", packet.buffer); //DEBUG
		func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
		if (func_return < 0) {
			printf("ERROR sendto F_NAME_REQ\n");
		}

	//Recebe confirmação do servidor
	func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
	if (func_return < 0) {
		printf("ERROR recvfrom from delete reply\n");
	}
	if(strcmp(packet.buffer, DEL_COMPLETE) == 0){
		printf("Arquivo deletado!");
	}
*/
}

void close_session() { //TODO: corrigir segmentation fault 
/*
	Frame packet;
	bzero(packet.user, MAXNAME-1);
	strcpy(packet.user, user.id);
	bzero(packet.buffer, BUFFER_SIZE -1);
	packet.ack = FALSE;

	//Fecha a thread de sincronizacao. A thread ainda nao foi criada.
	//pthread_cancel(sync_thread);

	//Finaliza thread do servidor
	strcpy(packet.buffer, END_REQ);
	func_return = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_conn, sizeof(struct sockaddr_in));
	if (func_return < 0) {
		printf("ERROR sendto END_REQ\n");
	}

	//Fecha o socket do cliente
	close(user.socket_id);
*/
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
				send_file_client(attribute);
			}
			/* DOWNLOAD */
			else if(strcmp(command, "download") == 0) {
				get_file(attribute);
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
