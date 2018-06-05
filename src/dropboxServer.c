#ifndef SERVER_CODE
#define SERVER_CODE

#include "dropboxServer.h"
#include "sync-server.h"



/* Used to sync server files */
void sync_server(int sock_s, Client *client_s) {

	synchronize_client(sock_s, client_s);

	synchronize_server(sock_s, client_s);
}


void receive_file(char* filename, int sockid, char* id) {
	char filepath[3*MAXNAME];
	int bytes_received;
	int file_size;
	
	char buffer[BUFFER_SIZE];

	struct sockaddr_in cli_addr;

	sprintf(filepath, "%s/%s/%s/%s", getUserHome(), SERVER_FOLDER, id, filename);
	printf("\nReceiving file at %s", filepath); //DEBUG
	FILE* file;
	file = fopen(filepath, "wb");

	if(file) {
		/* Receives the file size from client*/
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving file size from client");
		
		file_size = atoi(buffer);
		if(file_size == 0) {
			fclose(file);
			return;
		}

		/* Receives the file in BUFFER_SIZE sized parts*/
		bytes_received = 0;
		while(file_size > bytes_received) {
		
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR receiving file from client");		
			
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
		printf("\nErro ao abrir o arquivo %s\n", filepath);
}


void send_file_server(char *filename, int sockid, char* id, struct sockaddr_in *cli_addr) {
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
			return;
		}

		sprintf(buffer, "%d", file_size);
		printf("\nFile size: %s", buffer);

		/* Sends the file size to the client*/
		if(send_packet(START_MSG_COUNTER, buffer, sockid, cli_addr) < 0)
			printf("\nERROR sending file size to client");

		/* Sends the file in BUFFER_SIZE sized parts*/
		bytes_sent = 0;
		while(!feof(file)) {
			fread(buffer, sizeof(char), BUFFER_SIZE, file);
			bytes_sent += sizeof(char) * BUFFER_SIZE;
			
			if(send_packet(START_MSG_COUNTER, buffer, sockid, cli_addr) < 0)
			printf("\nERROR sending file to client");

			printf("\n Sending file %s - Total: %d / Read: %d", filename, file_size, bytes_sent); //DEBUG
		}
		printf("\n Finished sending file %s\n", filename);
		fclose(file);
	}
	else
		printf("\nErro ao abrir o arquivo %s\n", filepath);
}

#endif
