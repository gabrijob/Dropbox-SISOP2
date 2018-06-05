#ifndef SYNC_CLIENT_CODE
#define SYNC_CLIENT_CODE


/* Defining constants to the watcher thread */

#include "sync-client.h"


void synchronize_local(UserInfo *user) {

	FileInfo localFiles[MAXFILES];
	char path[MAXPATH];
	//char filename[MAXNAME];
	char last_modified_server[MAXNAME];
	char last_modified_client[MAXNAME];
	char buffer[BUFFER_SIZE];

	int number_files_client;
	//int status;
	//unsigned int length;

	int sockid = user->socket_id;

	struct sockaddr_in* serv_addr = user->serv_conn;
	//struct sockaddr_in from, to;
	//Frame packet;
	//length = sizeof(struct sockaddr_in);

	printf("Starting local sync-client\n");		//debug
	

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
		printf("Name sent: %s\n", localFiles[i].name);	//debug

		if(send_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR sending file name");


		/* Receive update or deletion request from server */
		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
			printf("\nERROR receiving request from server");


		/* Start update procedure */
		if(strcmp(buffer, UPDATE_PROC) == 0) {
			/* Receive file's last modification at server */
			bzero(buffer, BUFFER_SIZE);
			if(recv_packet(START_MSG_COUNTER, buffer, sockid, serv_addr) < 0)
				printf("\nERROR receiving file's last modification");

			strcpy(last_modified_server, buffer);
			printf("Last modified: %s\n", last_modified_server);  //debug


			/* Function to acquire modification time of sync file */
			getModifiedTime(path, last_modified_client);


			/* Test whether file at client is older or newer and downloads or uploads it */
			if(check_dir(path) == FALSE) {;					
				printf("File %s does not exist... downloading\n", localFiles[i].name);	//debug
				get_file(localFiles[i].name, user, user->folder);
			}
			else if (older_file(last_modified_server, last_modified_client) == SUCCESS) {
				printf("File %s older... downloading\n", localFiles[i].name);	//debug
				get_file(localFiles[i].name, user, user->folder);
			}
			else {
				printf("File %s newer... uploading\n", localFiles[i].name);	//debug
				send_file_client(localFiles[i].name, user);
			}

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
	
	printf("End of local sync (client)\n");		//debug
	
	
	//strcpy(packet.buffer, S_NSYNC);
	
	
	/* Getting an ACK */
	/* SYNC	*/
	/*while (strcmp(packet.buffer, S_SYNC) != 0) {

		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));

		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &to, &length);
	}
	if (status < 0) {
			printf("ERROR reading from socket in sync-client local\n");
	}

	do {
		/* Receive the number of files at server */
		/*status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		if(strcmp(packet.user, SERVER_USER)==0){
			packet.ack = TRUE;
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		}
	}while(packet.ack != TRUE);
	
	if (status < 0) {
		printf("ERROR reading from socket in sync-client local\n");
	}

	number_files_server = atoi(packet.buffer);
	printf("%d arquivos no servidor\n", number_files_server); //debug

	for(int i = 0; i < number_files_server; i++) {
		do { 
		    /* Receives file name from server */
		    /*status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		    if(strcmp(packet.user, SERVER_USER)==0){
			 packet.ack = TRUE;
			 status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		    }

		}while(packet.ack != TRUE);

		if (status < 0) {
			printf("ERROR reading from socket in sync-client\n");
		}

		strcpy(file_name, packet.buffer);
		printf("Nome recebido: %s\n", file_name);			//debug

		do {
		    /* Receives the date of last file modification from server */
		    /*status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		    if(strcmp(packet.user, SERVER_USER)==0){
			 packet.ack = TRUE;
			 status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		    }

		}while(packet.ack != TRUE); 

		if (status < 0) {
			printf("ERROR reading from socket in sync-client\n");	//debug
		}

		strcpy(last_modified, packet.buffer);
		printf("Last modified: %s\n", last_modified);			//debug

		sprintf(path, "%s/%s", user->folder, file_name);

		/* Function to acquire modification time of sync file */
		/*getModifiedTime(path, last_modified_file_2);

		if(check_dir(path) == FALSE) {;					
			printf("File %s does not exist... downloading\n", file_name);	//debug
			//get_file(file_name, user);

		} else if (older_file(last_modified, last_modified_file_2) == SUCCESS) {
			printf("File %s older... downloading\n", file_name);	//debug
			//get_file(file_name, user);

		} else {
			strcpy(packet.buffer, S_OK);
			do { /* ACK -> confirming OK! */
				/*status=sendto(sockid,&packet,sizeof(packet),0,(const struct sockaddr *) serv_addr,sizeof(struct sockaddr_in));
				status=recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);

			}while(strcmp(packet.buffer, S_OK) != 0 || (packet.ack == FALSE));

			if (status < 0) {
				printf("ERROR writing to socket in sync-client\n");
			}
		}	
	}*/
}


void synchronize_remote(UserInfo *user) {

	FileInfo localFiles[MAXFILES];
	char path[MAXPATH];
	int number_files_client = 0;
	int status;

	Frame packet;
	struct sockaddr_in from;

	int sockid = user->socket_id;

	struct sockaddr_in* serv_addr = user->serv_conn;
	unsigned int length = sizeof(struct sockaddr_in);

	printf("\nStarting server sync\n");	//debug

	number_files_client = get_dir_file_info(user->folder, localFiles);
	sprintf(packet.buffer, "%d", number_files_client);
	packet.ack = FALSE;

	/* Getting an ACK */
	while (packet.ack == FALSE) {

		/* Sends the number of files to server */
		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length); 
		if (status < 0) {
			printf("ERROR reading from socket in sync-client remote\n");
		}
	}

	for(int i = 0; i < number_files_client; i++) {
		packet.ack = FALSE;

		strcpy(packet.buffer, localFiles[i].name);
		printf("Name sent: %s\n", localFiles[i].name);	//debug

		do { /* ACK */
			/* Sends file's name to server */
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		}while(packet.ack != TRUE || (strcmp(packet.user, SERVER_USER) != 0));

		if (status < 0) {
			printf("ERROR writing to socket\n");
		}

		strcpy(packet.buffer, localFiles[i].last_modified);
		printf("Last modified: %s\n", localFiles[i].last_modified);	//debug
		packet.ack = FALSE;

		do { /* ACK */
			/* Sends last modified to server */
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		}while(packet.ack != TRUE || (strcmp(packet.user, SERVER_USER) != 0));

		if (status < 0) {
			printf("ERROR writing to socket on sync client\n");	//debug
		}
		
		/* Receives the answer if server will download file or not */
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		if (status < 0) {
			printf("ERROR reading from socket\n");
		}

		printf("Received: %s\n", packet.buffer);	//debug

		if(strcmp(packet.buffer, S_GET) == 0) {
			sprintf(path, "%s/%s", user->folder, localFiles[i].name);
			send_file_client(path, user);
		}
		else { /*ACK*/
			packet.ack = TRUE;
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));
		}
	}

	printf("\nEnd of server sync\n");		//debug
}

#endif
