#ifndef SYNC_SERVER_CODE
#define SYNC_SERVER_CODE

#include "sync-server.h"


void synchronize_client(int sockid, Client* client_sync) { 

	char buffer[BUFFER_SIZE];	
	int number_files_server;

	struct sockaddr_in cli_addr;

	printf("\nIniciando sincronização do cliente.\n");	//debug

	/* Receive answer from client */
	bzero(buffer, BUFFER_SIZE);
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR receiving start message from client");	
	
	
	number_files_server = client_sync->n_files;	

	/* Send number of files on server */
	sprintf(buffer, "%d", number_files_server);
	printf("\nClient number of files: %s.", buffer);	//debug
	if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR sending number of files");
	

	//for each file on server
	for(int i = 0; i < number_files_server; i++) {    
		/* Send file name */
		strcpy(buffer, client_sync->files[i].name);
		printf("\nNome do arquivo a enviar: %s", buffer);	//debug		
		if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR sending file name");
		
		
		/* Sends the file's last modification */
		strcpy(buffer, client_sync->files[i].last_modified);
		printf("\nLast modified: %s", buffer);	//debug
		if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR sending file's last_modification");


		/* Receive answer from client */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving answer from client");	

		printf("\nReceived: %s", buffer);
		/* Send file if client requested it*/
		if(strcmp(buffer, DOWN_REQ) == 0){
			/* Tell client that file name is not required */
			strcpy(buffer, F_NAME_NREQ);
			if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR sending file's last_modification"); 
		   	send_file_server(client_sync->files[i].name, sockid, client_sync->userid, &cli_addr);
		}
	}	
	printf("\nEncerrando sincronização do cliente.\n");		
}


void synchronize_server(int sockid, Client* client_sync) {

	char buffer[BUFFER_SIZE];
	char filename[MAXNAME];
	char filepath[4*MAXNAME];
	char last_modified_client[MAXNAME];
	char last_modified_server[MAXNAME];

	struct sockaddr_in cli_addr;


	printf("\nIniciando sincronização do servidor.\n");	//debug

	/* Receives number of files on client */
	bzero(buffer, BUFFER_SIZE);
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
		printf("\nERROR receiving number of files from client");

	int number_files_client = atoi(buffer);
	printf("\nNumber of files no cliente: %d", number_files_client); //debug
	
	//for each file on client
	for(int i = 0; i < number_files_client; i++) {
		/* Receive file name */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving file name");
		
		strcpy(filename, buffer);
	    printf("\nNome recebido: %s", filename);		//debug
		

		/* Receive last modified at client */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
			printf("\nERROR receiving last modified at client");

		printf("\nLast modified recebido: %s", buffer);
		strcpy(last_modified_client, buffer);

		sprintf(filepath, "%s/%s/%s/%s", getUserHome(), SERVER_FOLDER, client_sync->userid, filename);
    	getModifiedTime(filepath, last_modified_server);
	    

		/* Asks for file if it's older or doesn't exist */
		if(check_dir(filepath) == FALSE) {
			/* Send delete request to client */
			strcpy(buffer, DEL_REQ);
			if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR requesting file deletion");


			/* Receive delete confirmation from client */
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR receiving delete confirmation");

			if(strcmp(buffer, DEL_COMPLETE) == 0)
				printf("\nFile: %s deleted at client", filename); //debug
		} 
		else if(older_file(last_modified_client, last_modified_server) == 1) {
			/* Message client to start an upload */
			strcpy(buffer, S_GET);
			if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR requesting file from client");


			/* Receives request to upload from client  */
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR receiving upload request from client");
			
			printf("\nRecebido: %s", buffer);	//buffer
			/* Uploads file */
			if(strcmp(buffer, UP_REQ) == 0) {
				/* Tell client that file name is not required */
				strcpy(buffer, F_NAME_NREQ);
				if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
					printf("\nERROR sending file's last_modification");

				receive_file(filename, sockid, client_sync->userid);
			}
		}
		/* If neither send OK message*/
		else {
			strcpy(buffer, S_OK);
			if(send_packet(START_MSG_COUNTER, buffer, sockid, &cli_addr) < 0)
				printf("\nERROR sending OK message");
		}
	}
	printf("\nEncerrando sincronização do servidor.\n");	//debug
	
}

#endif