#ifndef SERVER_CODE
#define SERVER_CODE

#include "dropboxServer.h"
#include "sync-server.h"



/* Used to sync server files */
void sync_server(int sock_s, Client *client_s, MSG_ID* msg_id) {

	synchronize_client(sock_s, client_s, msg_id);

	synchronize_server(sock_s, client_s, msg_id);
}


void receive_file(char* filename, int sockid, char* id, MSG_ID* msg_id, pthread_mutex_t *file_mutex) {
	char filepath[3*MAXNAME];
	int bytes_received;
	int file_size;
	
	char buffer[BUFFER_SIZE];

	struct sockaddr_in cli_addr;

	sprintf(filepath, "%s/%s/%s/%s", getUserHome(), SERVER_FOLDER, id, filename);
	printf("\nReceiving file at %s", filepath); //DEBUG

	/* Receives the file size from client*/
	if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR receiving file size from client");
		
	file_size = atoi(buffer);


	FILE* file;
	file = fopen(filepath, "wb");

	if(file) {
		/* Tell client to start sending if size > 0*/
		if(file_size == 0) {
			strcpy(buffer, NOT_OK);
			if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR sending message to not send file\n");
			return;
		}
		else {
			strcpy(buffer, OK);
			if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR sending confirmation to server\n");
		}


		/* Receives the file in BUFFER_SIZE sized parts*/
		bytes_received = 0;
		pthread_mutex_lock(file_mutex);
		while(file_size > bytes_received) {
		
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR receiving file from client");		
			
			//printf("\nMSG ID = %d", msg_id->client); //debug
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
		pthread_mutex_unlock(file_mutex); 
		printf("\n Finished receiving file %s\n", filename); //DEBUG
		fclose(file);
	}
	else {
		printf("\nErro ao abrir o arquivo %s\n", filepath);
	
		/* Tell client to not send file */
		strcpy(buffer, NOT_OK);
		if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR sending message to not send file\n");
	}
}


void send_file(char *filename, int sockid, char* id, struct sockaddr_in *cli_addr, MSG_ID* msg_id, pthread_mutex_t *file_mutex) {
	char filepath[3*MAXNAME];
	int bytes_sent;
	int file_size;

	char buffer[BUFFER_SIZE];


	sprintf(filepath, "%s/%s/%s/%s", getUserHome(), SERVER_FOLDER, id, filename);
	printf("\nSending file at %s", filepath); //DEBUG


	FILE* file;
	file = fopen(filepath, "rb");
	
	if(file) {
		file_size = getFilesize(file);
		if(file_size == 0) {
			fclose(file);
			printf("\nThe file is empty");
		}			

		sprintf(buffer, "%d", file_size);
		printf("\nFile size: %s", buffer);

		/* Sends the file size to the client*/
		if(send_packet(&msg_id->server, buffer, sockid, cli_addr) < 0)
			printf("\nERROR sending file size to client");

		/* Receive confirmation to start sending */
		if(recv_packet(&msg_id->client, buffer, sockid, cli_addr) < 0)
			printf("\nERROR receiving confirmation to send");
		
		if(strcmp(buffer, NOT_OK) == 0) return;

		/* Sends the file in BUFFER_SIZE sized parts */
		bytes_sent = 0;
		pthread_mutex_lock(file_mutex);
		while(!feof(file)) {
			fread(buffer, sizeof(char), BUFFER_SIZE, file);
			bytes_sent += sizeof(char) * BUFFER_SIZE;
			
			if(send_packet(&msg_id->server, buffer, sockid, cli_addr) < 0)
				printf("\nERROR sending file to client");

			//printf("\nMSG ID = %d", msg_id->server); //debug
			printf("\n Sending file %s - Total: %d / Read: %d", filename, file_size, bytes_sent); //DEBUG
		}
		pthread_mutex_unlock(file_mutex);
		printf("\n Finished sending file %s\n", filename);
		fclose(file);
	}
	else {
		printf("\nErro ao abrir o arquivo %s\n", filepath);

		/* Sends the file size 0 to the client*/
		sprintf(buffer, "0");
		if(send_packet(&msg_id->server, buffer, sockid, cli_addr) < 0)
			printf("\nERROR sending file size 0 to client");

		/* Receive message to not send */
		if(recv_packet(&msg_id->client, buffer, sockid, cli_addr) < 0)
			printf("\nERROR receiving not send message");
	}
}

#endif
