#ifndef SYNC_CLIENT_CODE
#define SYNC_CLIENT_CODE


/* Defining constants to the watcher thread */

#include "sync-client.h"


void synchronize_local(UserInfo *user) {

	char path[MAXPATH];
	char filename[MAXNAME];
	char last_modified_server[MAXNAME];
	char last_modified_client[MAXNAME];
	char buffer[BUFFER_SIZE];

	int number_files_server;


	int sockid = user->socket_id;

	struct sockaddr_in* serv_addr = user->serv_conn;

	printf("Starting local sync-client\n");		//debug
	
	/* Send start sync message */
	strcpy(buffer, S_SYNC);
	if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
		printf("\nERROR sending message to start sync");


	/* Receives number of files on server */
	bzero(buffer, BUFFER_SIZE);	
	if(recv_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
		printf("\nERROR receiving number of files at server");

	number_files_server = atoi(buffer);
	printf("%d arquivos no servidor\n", number_files_server); //debug
	

	//for each file on server
	for(int i = 0; i < number_files_server; i++) {

		/* Receive file name */
		bzero(buffer, BUFFER_SIZE);	
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR receiving file name");

		strcpy(filename, buffer);
		printf("Nome recebido: %s\n", filename);			//debug


		/* Receive file's last modification at server */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR receiving file's last modification");

		strcpy(last_modified_server, buffer);
		printf("Last modified: %s\n", last_modified_server);			//debug


		sprintf(path, "%s/%s", user->folder, filename);
		/* Function to acquire modification time of sync file */
		getModifiedTime(path, last_modified_client);

		/* Asks for file if it's older or doesn't exist */
		if(check_dir(path) == FALSE) {;					
			printf("File %s does not exist... downloading\n", filename);	//debug
			get_file(filename, user, user->folder);
		}
		else if (older_file(last_modified_server, last_modified_client) == 0) {
			printf("File %s older... downloading\n", filename);	//debug
			get_file(filename, user, user->folder);
		}
		/* If neither send OK message*/
		else {
			strcpy(buffer, S_OK);
			if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
				printf("\nERROR sending OK message");
		}
	}
	
	printf("End of local sync (client)\n");		//debug
}


void synchronize_remote(UserInfo *user) {

	FileInfo localFiles[MAXFILES];
	char path[MAXPATH];
	char buffer[BUFFER_SIZE];

	int number_files_client;

	int sockid = user->socket_id;

	struct sockaddr_in* serv_addr = user->serv_conn;


	printf("\nStarting server sync");		//debug
	
	/* Sends number of files on client */
	number_files_client = get_dir_file_info(user->folder, localFiles);
	sprintf(buffer, "%d", number_files_client);
	
	if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
		printf("\nERROR sending number of files");


	//for each file on client
	for(int i = 0; i < number_files_client; i++) {
		sprintf(path, "%s/%s", user->folder, localFiles[i].name);

		/* Send file name */
		strcpy(buffer, localFiles[i].name);
		printf("\nName sent: %s", buffer);	//debug

		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR sending file name");


		/* Send last modified to server */
		strcpy(buffer, localFiles[i].last_modified);
		printf("\nLast modified: %s", buffer);	//debug

		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR sending last modified");


		/* Receive update or deletion request from server */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR receiving request from server");

		printf("\nReceived: %s", buffer);	//debug

		/* Send file */
		if(strcmp(buffer, S_GET) == 0) {
			send_file_client(path, user);
		}
		/* Delete file */
		else if(strcmp(buffer, DEL_REQ) == 0) {
			int deleted;

			/* Removes file */			
			if(fileExists(path)) { 
				if(remove(path) != 0) {
					printf("\nErro ao deletar o arquivo %s", localFiles[i].name);
					deleted = ERROR;
				}
				else {
					printf("\nArquivo %s removido", localFiles[i].name);
					deleted = SUCCESS;
				}
			}
			if(deleted == SUCCESS)
				strcpy(buffer, DEL_COMPLETE);

			/* Send delete confirmation to server */
			if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR sending delete confimation");
		}

	}
	
	printf("\nEnd of server sync\n");		//debug

}

#endif
