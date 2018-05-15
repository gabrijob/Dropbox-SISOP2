#ifndef SYNC_CLIENT_CODE
#define SYNC_CLIENT_CODE


/* Defining constants to the watcher thread */

#include "sync-client.h"


void synchronize_local(UserInfo *user) {

	char path[MAXPATH];
	char file_name[MAXNAME];
	char last_modified[MAXNAME];
	char last_modified_file_2[MAXNAME];

	int number_files_server = 0;
	int status;
	unsigned int length;

	int sockid = user->socket_id;

	struct sockaddr_in* serv_addr = user->serv_conn;
	struct sockaddr_in from, to;
	Frame packet;
	length = sizeof(struct sockaddr_in);

	printf("Starting local sync-client\n");		//debug
	strcpy(packet.buffer, S_NSYNC);
	
	
	/* Getting an ACK */
	/* SYNC	*/
	while (strcmp(packet.buffer, S_SYNC) != 0) {

		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in));

		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &to, &length);
	}
	if (status < 0) {
			printf("ERROR reading from socket in sync-client local\n");
	}

	do {
		/* Receive the number of files at server */
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
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
		    status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
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
		    status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
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
		getModifiedTime(path, last_modified_file_2);

		if(check_dir(path) == FALSE) {;					
			printf("File %s does not exist... downloading\n", file_name);	//debug
			get_file(file_name, user);

		} else if (older_file(last_modified, last_modified_file_2) == SUCCESS) {
			printf("File %s older... downloading\n", file_name);	//debug
			get_file(file_name, user);

		} else {
			strcpy(packet.buffer, S_OK);
			do { /* ACK -> confirming OK! */
				status=sendto(sockid,&packet,sizeof(packet),0,(const struct sockaddr *) serv_addr,sizeof(struct sockaddr_in));
				status=recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);

			}while(strcmp(packet.buffer, S_OK) != 0 || (packet.ack == FALSE));

			if (status < 0) {
				printf("ERROR writing to socket in sync-client\n");
			}
		}	
	}

	printf("End of local sync (client)\n");		//debug
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