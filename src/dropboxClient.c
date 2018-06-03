#ifndef CLIENT_CODE
#define CLIENT_CODE

#include "dropboxClient.h"
#include "sys/socket.h" 
#include "arpa/inet.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/*   Global variables   */
int ID_MSG_CLIENT = START_MSG_COUNTER;
int ID_MSG_SERVER = START_MSG_COUNTER;


//Start of client interface

int login_server(char *host, int port, UserInfo *user) {
	int sockid;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in serv_conn, from;
		
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockid == ERROR) {
		printf("Error opening socket ");
		return ERROR;
	}
	else
		printf("First client socket %i\n", sockid);
	
	user->socket_id = sockid;

	bzero((char *) &serv_conn, sizeof(serv_conn));

	serv_conn.sin_family = AF_INET;
	serv_conn.sin_port = htons(port);
	serv_conn.sin_addr.s_addr = inet_addr(host);
	user->serv_conn = &serv_conn;
	

	/* Sends first message to server */
	strcpy(buffer, user->id);
	ID_MSG_CLIENT = START_MSG_COUNTER;
	if(send_packet(ID_MSG_CLIENT, buffer, sockid, &serv_conn) < 0) {
		printf("\nERROR starting connection with server"); 
		return ERROR;
	}
	ID_MSG_CLIENT++;

	/* Receives new port for connection */
	bzero(buffer, BUFFER_SIZE -1);
	ID_MSG_SERVER = START_MSG_COUNTER;
	if(recv_packet(ID_MSG_SERVER, buffer, sockid, &from) < 0) {
		printf("\nERROR starting connection with server"); 
		return ERROR;
	}
	ID_MSG_SERVER++;

	serv_conn.sin_port = htons(atoi(buffer)); //updates default port with port received from the server

	/* Initializes mutex to control comunication with server*/
	if(pthread_mutex_init(&user->lock_server_comm, NULL) != 0){
		printf("ERROR mutex init failed ");
		return ERROR;
	}

	/* Sync the files from user to server */
	sync_client(user); 

	return SUCCESS;
}


void sync_client(UserInfo *user) {
	/* verifies if user folder exists */
	if(check_dir(user->folder) == FALSE) {
		if(mkdir(user->folder, 0777) != 0) {
			printf("Error creating user folder '%s'.\n", user->folder);
		}
	}

	//synchronize_local(user);

	//synchronize_remote(user);
}


void send_file_client(char *path, UserInfo *user) {
	char* filename;
	char filepath[MAXPATH];
	int file_size;
	int bytes_sent;
	
	strcpy(filepath, path);
	filename = basename(path);

	int sockid = user->socket_id;
	struct sockaddr_in* serv_conn = user->serv_conn;

	char buffer[BUFFER_SIZE];

	printf("\nRequisitando envio de arquivo para porta-%d/socket-%d", ntohs(serv_conn->sin_port), sockid); //DEBUG
	
	/* Sends upload request to server */
	strcpy(buffer, UP_REQ);

	if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0) {
		printf("\nERROR sending upload request");
		return;
	}

	/* Receive ack from server */
	struct sockaddr_in from;

	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, &from) < 0) {
		printf("\nERROR receiving file name request");
		return;
	}

	if(strcmp(buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(buffer, filename);
		printf("\nEnviando arquivo: %s\n", buffer); //DEBUG
	
		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0) {
			printf("\nERROR sending file name");
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
		
		sprintf(buffer, "%d", file_size);
		/* Sends the file size to the server*/
		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0)
			printf("\nERROR sending file size");

		/* Sends the file in BUFFER_SIZE sized parts*/
		bytes_sent = 0;
		while(!feof(file)) {
			fread(buffer, sizeof(char), BUFFER_SIZE, file);
			bytes_sent += sizeof(char) * BUFFER_SIZE;

			if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0)
				printf("\nERROR sending file");

			printf("\n Sending file %s - Total: %d / Read: %d", filepath, file_size, bytes_sent); //DEBUG
		}
		printf("\n Finished sending file %s\n", filepath);
		fclose(file);
	}
	else
		printf("\nErro ao abrir o arquivo %s\n", filepath);
}


void get_file(char *filename, UserInfo *user) {
	int file_size;
	int bytes_received;

	int sockid = user->socket_id;
	struct sockaddr_in* serv_conn = user->serv_conn;

	char buffer[BUFFER_SIZE];

	/* Sends download request to server */
	strcpy(buffer, DOWN_REQ);

	if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0) {
		printf("\nERROR sending download request");
		return;
	}

	/* Receive ack from server */
	struct sockaddr_in from;

	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, &from) < 0) {
		printf("\nERROR receiving file name request");
		return;
	}

	if(strcmp(buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(buffer, filename);
		printf("\nPedindo arquivo: %s\n", buffer); //DEBUG

		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0) {
			printf("\nERROR sending file name");
			return;
		}
	}

	char filepath[3*MAXNAME];
	sprintf(filepath, "%s/%s", user->folder, filename);
	printf("File at %s for download", filepath); //DEBUG
	FILE* file;
	file = fopen(filepath, "wb");

	if(file) {
		/* Receives the file size from server */
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, &from) < 0)
			printf("ERROR receiving file size from server\n");

		file_size = atoi(buffer);		

		bytes_received = 0;
		/* Receives the file in BUFFER_SIZE sized parts*/
		while(file_size > bytes_received) {
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(START_MSG_COUNTER, buffer, sockid, &from) < 0)
				printf("ERROR receiving file from server\n");

			if((file_size - bytes_received) > BUFFER_SIZE) {
				fwrite(buffer, sizeof(char), BUFFER_SIZE, file);
				bytes_received += sizeof(char) * BUFFER_SIZE; 
			}
			else {
				fwrite(buffer, sizeof(char), (file_size - bytes_received), file);
				bytes_received += sizeof(char) * (file_size - bytes_received);
			}
			printf("\n Receiving file %s - Total: %d / Written: %d", filename, file_size, bytes_received); //DEBUG
		}
		printf("\n Finished receiving file %s\n", filename); //DEBUG
		fclose(file);
	}
	else
		printf("\nErro ao abrir o arquivo %s", filepath);
}


void delete_file(char *filename, UserInfo *user) {
	int sockid = user->socket_id;
	struct sockaddr_in *serv_conn = user->serv_conn;

	char buffer[BUFFER_SIZE];

	//Send delete request to server
	strcpy(buffer, DEL_REQ);

	if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0) {
		printf("\n ERROR sending delete request");
		return;
	}

	//Receive ack from server 
	struct sockaddr_in from;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, &from) < 0) {
		printf("\nERROR receiving file name request");
		return;
	}

	//Mandando o nome do arquivo pro servidor
	if(strcmp(buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(buffer, filename);
		printf("\nDeletando arquivo: %s", buffer); //DEBUG

		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0) {
			printf("\nERROR sending file name");
			return;
		}
	}	

	//Recebe confirmação do servidor
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, &from) < 0) {
		printf("\nERROR receiving delete confirmation");
		return;
	}
	if(strcmp(buffer, DEL_COMPLETE) == 0){
		printf("\nArquivo deletado!");
	}
}


void close_session(UserInfo *user) { //TODO: corrigir segmentation fault 

	char buffer[BUFFER_SIZE];

	int sockid = user->socket_id;
	struct sockaddr_in *serv_conn = user->serv_conn;
	
	//Destroi mutex
	pthread_mutex_destroy(&user->lock_server_comm);

	//Finaliza thread do servidor
	strcpy(buffer, END_REQ);
	if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_conn) < 0)
		printf("ERROR requesting session end");

	//Fecha o socket do cliente
	close(user->socket_id);

}

#endif
