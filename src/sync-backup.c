#ifndef SYNC_BACKUP_CODE
#define SYNC_BACKUP_CODE

#include "sync-backup.h"

void synchronize_local(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id);
void synchronize_remote(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id);
void sync_to_backup_dw(int sockid, char* client_id, MSG_ID* msg_id);
void sync_to_backup_re(int sockid, char* client_id, MSG_ID* msg_id);



void send_all_clients(int sockid, struct sockaddr_in* backup_addr) {
    
    int zero;
    char buffer[BUFFER_SIZE];
    char prim_folder[3*MAXNAME];
    sprintf(prim_folder, "%s/%s", getUserHome(), SERVER_FOLDER);
    
    FileInfo files[MAXFILES];
    int counter;
    counter = get_dir_file_info(prim_folder, files);

    printf("\nSending all files to new backup server...");

    for (int i = 0; i < counter; i++) {
        /*If directory*/
        if(files[i].extension[0] == '\0') {
            /* Tell backup to complete sync */
            strcpy(buffer, TSO);
            zero = START_MSG_COUNTER;
            if(send_packet(&zero, buffer, sockid, backup_addr) < 0)
                printf("\nERROR telling backup server to complete sync");

            send_client_files(sockid, files[i].name, backup_addr);
        }
    }

    /* Tell backup that complete sync is over */
    strcpy(buffer, TSF);
    zero = START_MSG_COUNTER;
    if(send_packet(&zero, buffer, sockid, backup_addr) < 0)
        printf("\nERROR telling backup serve stop");

    printf("\n...Done\n");

}

void get_all_clients(int sockid) {

    int zero;
    struct sockaddr_in prim_addr;

    char buffer[BUFFER_SIZE];

    char backup_folder[3*MAXNAME];
    sprintf(backup_folder, "%s/%s", getUserHome(), /*TEST_*/SERVER_FOLDER);

    printf("\nReceiving all files from primary server...");

    /* Receive message to complete sync or not  */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_addr) < 0)
		printf("\nERROR receiving complete sync request"); 

    while(strcmp(buffer, TSF) != 0) {
        get_client_files(sockid);

        /* Receive message to complete sync or not  */
	    zero = START_MSG_COUNTER;
	    bzero(buffer, BUFFER_SIZE -1);
	    if(recv_packet(&zero, buffer, sockid, &prim_addr) < 0)
		    printf("\nERROR receiving sync status");
    }

    printf("\n...Done\n");
}



void send_client_files(int sockid, char* client_id, struct sockaddr_in* backup_addr) {
    int zero;
    char buffer[BUFFER_SIZE];

    /* Send client id */
	strcpy(buffer, client_id);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, sockid, backup_addr) < 0)
		printf("\nERROR sending client id to backup server");	
    
    /* Start synchronizing*/
    MSG_ID msg_id;
    msg_id.client = 0;
    msg_id.server = 0;
    
    printf("\nSynchronizing client %s with backup server...", client_id);

    sync_to_backup_dw(sockid, client_id, &msg_id);
    sync_to_backup_re(sockid, client_id, &msg_id);

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

    printf("\nSynchronizing client %s with primary server...", client_id);

    synchronize_local(sockid, &prim_addr, client_id, &msg_id);
    synchronize_remote(sockid, &prim_addr, client_id, &msg_id);

    printf("\n...Done\n");
	
}


void get_file(char *filename, int sockid, struct sockaddr_in* prim_addr, char *client_id, MSG_ID *msg_id) {
	int file_size;
	int bytes_received;

    char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), /*TEST_*/SERVER_FOLDER, client_id);

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
	sprintf(client_folder, "%s/%s/%s", getUserHome(), /*TEST_*/SERVER_FOLDER, client_id);

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


//------------------------------------------------------PRIVATE-------------------------------------------------------------------------

void synchronize_local(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id) {

	char path[MAXPATH];
	char filename[MAXNAME];
	char last_modified_server[MAXNAME];
	char last_modified_client[MAXNAME];
	char buffer[BUFFER_SIZE];

	int number_files_server;

	char client_folder[3*MAXNAME];
    /* Create client directory if it doesn't exist */
	sprintf(client_folder, "%s/%s/%s", getUserHome(), /*TEST_*/SERVER_FOLDER, client_id);
    if(check_dir(client_folder) == FALSE) {
		if(mkdir(client_folder, 0777) != SUCCESS) {
			printf("Error creating client folder '%s'.\n", client_folder);
		}
	}


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
	sprintf(client_folder, "%s/%s/%s", getUserHome(), /*TEST_*/SERVER_FOLDER, client_id);
	
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

void sync_to_backup_dw(int sockid, char* client_id, MSG_ID* msg_id) { 

	char buffer[BUFFER_SIZE];
    pthread_mutex_t file_mutex;
    pthread_mutex_init(&file_mutex, NULL);	
	int number_files_server;
    FileInfo localFiles[MAXFILES];

	struct sockaddr_in cli_addr;

    char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), SERVER_FOLDER, client_id);

	printf("\nIniciando sincronização do cliente.\n");	//debug

	/* Receive start message from client */
	bzero(buffer, BUFFER_SIZE);
	if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR receiving start message from client");	
	
	number_files_server = get_dir_file_info(client_folder, localFiles);	

	/* Send number of files on server */
	sprintf(buffer, "%d", number_files_server);
	printf("\nClient number of files: %s.", buffer);	//debug
	if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR sending number of files");
	

	//for each file on server
	for(int i = 0; i < number_files_server; i++) {    
		/* Send file name */
		strcpy(buffer, localFiles[i].name);
		printf("\nNome do arquivo a enviar: %s", buffer);	//debug		
		if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR sending file name");
		

		/* Sends the file's last modification */
		strcpy(buffer, localFiles[i].last_modified);
		printf("\nLast modified: %s", buffer);	//debug
		if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR sending file's last_modification");


		/* Receive answer from client */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving answer from client");	

		printf("\nReceived: %s", buffer);
		/* Send file if client requested it*/
		if(strcmp(buffer, DOWN_REQ) == 0){
			/* Tell client that file name is not required */
			strcpy(buffer, F_NAME_NREQ);
			if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR sending file's last_modification"); 


		   	send_file(localFiles[i].name, sockid, client_id, &cli_addr, msg_id, &file_mutex);
		}
	}	
	printf("\nEncerrando sincronização do cliente.\n");		
}


void sync_to_backup_re(int sockid, char* client_id, MSG_ID* msg_id) {

	char buffer[BUFFER_SIZE];
    pthread_mutex_t file_mutex;
    pthread_mutex_init(&file_mutex, NULL);
	char filename[MAXNAME];
	char filepath[4*MAXNAME];
	char last_modified_client[MAXNAME];
	char last_modified_server[MAXNAME];

	struct sockaddr_in cli_addr;

    char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s/%s", getUserHome(), SERVER_FOLDER, client_id);

	printf("\nIniciando sincronização do servidor.\n");	//debug

	/* Receives number of files on client */
	bzero(buffer, BUFFER_SIZE);
	if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR receiving number of files from client");

	int number_files_client = atoi(buffer);
	printf("\nNumber of files no cliente: %d", number_files_client); //debug
	
	//for each file on client
	for(int i = 0; i < number_files_client; i++) {
		/* Receive file name */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving file name");
		
		strcpy(filename, buffer);
	    printf("\nNome recebido: %s", filename);		//debug
		

		/* Receive last modified at client */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving last modified at client");

		printf("\nLast modified recebido: %s", buffer);
		strcpy(last_modified_client, buffer);

		sprintf(filepath, "%s/%s/%s/%s", getUserHome(), SERVER_FOLDER, client_id, filename);
    	getModifiedTime(filepath, last_modified_server);
	    printf("\nLast modified servidor: %s", last_modified_server);

		/* Asks for file if it's older or doesn't exist */
		if(check_dir(filepath) == FALSE) {
			/* Send delete request to client */
			strcpy(buffer, DEL_REQ);
			if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR requesting file deletion");


			/* Receive delete confirmation from client */
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR receiving delete confirmation");

			if(strcmp(buffer, DEL_COMPLETE) == 0)
				printf("\nFile: %s deleted at client", filename); //debug
		} 
		else if(older_file(last_modified_client, last_modified_server) == 1) {
			/* Message client to start an upload */
			strcpy(buffer, S_GET);
			if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR requesting file from client");


			/* Receives request to upload from client  */
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(&msg_id->client, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR receiving upload request from client");
			

			printf("\nRecebido: %s", buffer);	//buffer
			/* Uploads file */
			if(strcmp(buffer, UP_REQ) == 0) {
				/* Tell client that file name is not required */
				strcpy(buffer, F_NAME_NREQ);
				if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
					printf("\nERROR sending file's last_modification");

				receive_file(filename, sockid, client_id, msg_id, &file_mutex);
			}
		}
		/* If neither send OK message*/
		else {
			strcpy(buffer, S_OK);
			if(send_packet(&msg_id->server, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR sending OK message");
		}
	}
	printf("\nEncerrando sincronização do servidor.\n");	//debug
}



#endif