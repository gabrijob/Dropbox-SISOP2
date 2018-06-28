#ifndef SYNC_BACKUP_CODE
#define SYNC_BACKUP_CODE

#include "sync-backup.h"


void get_file(char *filename, int sockid, struct sockaddr_in* prim_addr, char *client_id, MSG_ID *msg_id) {
	int file_size;
	int bytes_received;

    char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), TEST_SERVER_FOLDER, client_id);

	char buffer[BUFFER_SIZE];

	/* Sends download request to server */
	strcpy(buffer, DOWN_REQ);

	if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0) {
		printf("\nERROR sending download request");
		return;
	}

	/* Receive ack from server */
	struct sockaddr_in from;

	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&msg_id->server, buffer, sockid, &from) < 0) {
		printf("\nERROR receiving file name request");
		return;
	}

	if(strcmp(buffer, F_NAME_REQ) == 0) {
		//Pegar apenas o nome do arquivo ou o path ?
		strcpy(buffer, filename);
		printf("\nPedindo arquivo: %s\n", buffer); //DEBUG

		if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0) {
			printf("\nERROR sending file name");
			return;
		}
	}

	char filepath[MAXPATH];
	sprintf(filepath, "%s/%s", client_folder, filename);
	printf("\nFile will be donwloaded in %s", filepath); //DEBUG

	/* Receives the file size from server */
	if(recv_packet(&msg_id->server, buffer, sockid, &from) < 0)
		printf("\nERROR receiving file size from server\n");
	
	printf("\nFile size: %s", buffer);
	file_size = atoi(buffer);


	FILE* file;
	file = fopen(filepath, "wb");

	if(file) {
		/* Tell server to start sending if size > 0*/
		if(file_size == 0) {
			strcpy(buffer, NOT_OK);
			if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
				printf("\nERROR sending message to not send file\n");
			return;
		}
		else {
			strcpy(buffer, OK);
			if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
				printf("\nERROR sending confirmation to server\n");
		}

		bytes_received = 0;
		/* Receives the file in BUFFER_SIZE sized parts*/
		while(file_size > bytes_received) {
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(&msg_id->server, buffer, sockid, &from) < 0)
				printf("ERROR receiving file from server\n");

			//printf("\nMSG ID = %d", msg_id->server); //debug
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
	else {
		printf("\nErro ao abrir o arquivo %s", filepath);

		/* Tell server to not send file */
		strcpy(buffer, NOT_OK);
		if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
			printf("\nERROR sending message to not send file\n");
	
	}
}


int delete_file(char* filename, char* client_id) {
	char filepath[4*MAXNAME];
	int ret;

	char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), TEST_SERVER_FOLDER, client_id);

	sprintf(filepath, "%s/%s", client_folder, filename);
	printf("\nRemoving file at: %s", filepath);
	
	if(fileExists(filepath)) { 
		if(remove(filepath) != 0) {
			printf("\nErro ao deletar o arquivo %s", filename);
			ret = ERROR;
		}
		else {
			printf("\nArquivo %s removido", filename);
			ret = SUCCESS;
		}
	}

	return ret;
}



void send_client_files(int sockid, Client* client_sync, struct sockaddr_in* backup_addr) {
    int zero;
    char buffer[BUFFER_SIZE];

    /* Send client id */
	strcpy(buffer, client_sync->userid);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, sockid, backup_addr) < 0)
		printf("\nERROR sending client id to backup server");	
    
    /* Start synchronizing*/
    MSG_ID msg_id;
    msg_id.client = 0;
    msg_id.server = 0;
    
    printf("\nSynchronizing client with backup server...");

    synchronize_client(sockid, client_sync, &msg_id);
    synchronize_server(sockid, client_sync, &msg_id);

    printf("\n...Done\n");
}


void get_client_files(int sockid) {
    int zero;
    struct sockaddr_in prim_addr;
    char client_id[MAXNAME];

    char buffer[BUFFER_SIZE];

    /* Receives client id  */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_addr) < 0)
		printf("\nERROR receiving client id"); 
		
	strcpy(client_id, buffer);


    /* Start synchronizing*/
    MSG_ID msg_id;
    msg_id.client = 0;
    msg_id.server = 0;

    printf("\nSynchronizing client with primary server...");

    synchronize_local(sockid, &prim_addr, client_id, &msg_id);
    synchronize_remote(sockid, &prim_addr, client_id, &msg_id);

    printf("\n...Done\n");
	
}

void synchronize_local(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id) {

	char path[MAXPATH];
	char filename[MAXNAME];
	char last_modified_server[MAXNAME];
	char last_modified_client[MAXNAME];
	char buffer[BUFFER_SIZE];

	int number_files_server;

	char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), TEST_SERVER_FOLDER, client_id);


	/* Send start sync message */
	strcpy(buffer, S_SYNC);
	if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
		printf("\nERROR sending message to start sync");


	/* Receives number of files on server */
	bzero(buffer, BUFFER_SIZE);	
	if(recv_packet(&msg_id->server, buffer, sockid, prim_addr) < 0)
		printf("\nERROR receiving number of files at server");

	number_files_server = atoi(buffer);
	printf("\n%d arquivos no servidor", number_files_server); //debug
	

	//for each file on server
	for(int i = 0; i < number_files_server; i++) {

		/* Receive file name */
		bzero(buffer, BUFFER_SIZE);	
		if(recv_packet(&msg_id->server, buffer, sockid, prim_addr) < 0)
			printf("\nERROR receiving file name");

		strcpy(filename, buffer);
		printf("\nNome recebido: %s", filename);			//debug


		/* Receive file's last modification at server */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(&msg_id->server, buffer, sockid, prim_addr) < 0)
			printf("\nERROR receiving file's last modification");

		strcpy(last_modified_server, buffer);
		printf("\nLast modified on server: %s", last_modified_server);			//debug


		sprintf(path, "%s/%s", client_folder, filename);
		/* Function to acquire modification time of sync file */
		getModifiedTime(path, last_modified_client);
		printf("\nLast modified local: %s", last_modified_client);

		/* Asks for file if it's older or doesn't exist */
		if(check_dir(path) == FALSE) {;					
			printf("\nFile %s does not exist... downloading", filename);	//debug
			get_file(filename, sockid, prim_addr, client_id, msg_id);
		}
		else if (older_file(last_modified_server, last_modified_client) == 1) {
			printf("\nFile %s older... downloading", filename);	//debug
			get_file(filename, sockid, prim_addr, client_id, msg_id);
		}
		/* If neither send OK message*/
		else {
			strcpy(buffer, S_OK);
			if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
				printf("\nERROR sending OK message");
		}
	}
}


void synchronize_remote(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id) {

	FileInfo localFiles[MAXFILES];
	char path[MAXPATH];
	char buffer[BUFFER_SIZE];

	int number_files_client;


    char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), TEST_SERVER_FOLDER, client_id);
	
	/* Sends number of files on client */
	number_files_client = get_dir_file_info(client_folder, localFiles);
	sprintf(buffer, "%d", number_files_client);
	
	if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
		printf("\nERROR sending number of files");


	//for each file on client
	for(int i = 0; i < number_files_client; i++) {
		sprintf(path, "%s/%s", client_folder, localFiles[i].name);

		/* Send file name */
		strcpy(buffer, localFiles[i].name);
		printf("\nName sent: %s", buffer);	//debug

		if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
			printf("\nERROR sending file name");


		/* Send last modified to server */
		strcpy(buffer, localFiles[i].last_modified);
		printf("\nLast modified: %s", buffer);	//debug

		if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
			printf("\nERROR sending last modified");


		/* Receive delete request  or ok message from primary server */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(&msg_id->server, buffer, sockid, prim_addr) < 0)
			printf("\nERROR receiving request from server");

		printf("\nReceived: %s", buffer);	//debug

		/* Delete file */
		if(strcmp(buffer, DEL_REQ) == 0) {
			if(delete_file(localFiles[i].name, client_id) == SUCCESS)
				strcpy(buffer, DEL_COMPLETE);

			/* Send delete confirmation to server */
			if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
			    printf("\nERROR sending delete confimation");
		}
        else if(strcmp(buffer, S_GET) == 0) {
            strcpy(buffer, NOT_OK);
            /* Send recusation to upload to server */
			if(send_packet(&msg_id->client, buffer, sockid, prim_addr) < 0)
			    printf("\nERROR sending upload recusation");
        }

	}
}



#endif