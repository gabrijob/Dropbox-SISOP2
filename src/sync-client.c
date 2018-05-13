/* Defining constants to the watcher thread */
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE))

/* Communication constants */
#define S_SYNC "sync"
#define S_NSYNC "not_sync"
#define S_OK "ok"
#define S_GET "get"

#include "dropboxClient.h"

void synchronize_local(int sockid, struct sockaddr_in serv_addr, UserInfo user) {

	char path[MAXPATH];
	char file_name[MAXNAME];
	char last_modified[MAXNAME];
	char last_modified_file_2[MAXNAME];

	int number_files_server = 0;
	int status;
	unsigned int length;

	struct sockaddr_in from, to;
	Frame packet;
	length = sizeof(struct sockaddr_in);

	printf("Starting local sync-client\n");		//debug
	strcpy(packet.buffer, S_NSYNC);
	packet.ack == FALSE;
	
	/* Getting an ACK */
	/* SYNC	*/
	while (strcmp(packet.buffer, S_SYNC) != 0) {
		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
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
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
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
			 status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		}while(packet.ack != TRUE);

		if (status < 0) {
			DEBUG_PRINT("ERROR reading from socket in sync-client\n");
		}

		strcpy(file_name, packet.buffer);
		printf("Nome recebido: %s\n", file_name);			//debug

		do {
		    /* Receives the date of last file modification from server */
		    status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
		    if(strcmp(packet.user, SERVER_USER)==0){
			 packet.ack = TRUE;
			 status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		}while(packet.ack != TRUE); 

		if (status < 0) {
			printf("ERROR reading from socket in sync-client\n");	//debug
		}

		strcpy(last_modified, packet.buffer);
		printf("Last modified: %s\n", last_modified);			//debug

		sprintf(path, "%s/%s", user.folder, file_name);

		/* Function to acquire modification time of sync file */
		getModifiedTime(path, last_modified_file_2);

		if(check_dir(path) == FALSE) {
			//get_file(file_name, NULL);					//client interface implementation
			printf("File %s does not exist... downloading\n", file_name);	//debug

		} else if (older_file(last_modified, last_modified_file_2) == SUCCESS) {
			//get_file(file_name, NULL);
			printf("File %s older... downloading\n", file_name);	//debug

		} else {
			strcpy(packet.buffer, S_OK);
			do { /* ACK -> confirming OK! */
				status=sendto(sockid,&packet,sizeof(packet),0,(const struct sockaddr *)&serv_addr,sizeof(struct sockaddr_in));
				status=recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);

			}while(strcmp(packet.buffer, S_OK) != 0 || (packet.ack == FALSE));

			if (status < 0) {
				printf("ERROR writing to socket in sync-client\n");
			}
		}	
	}

	printf("End of local sync (client)\n");		//debug
}


void synchronize_remote(int sockid, struct sockaddr_in serv_addr, UserInfo user) {

	FileInfo localFiles[MAXFILES];
	char path[MAXPATH];
	int number_files_client = 0;
	int status;

	Frame packet;
	struct sockaddr_in from, to;

	unsigned int length = sizeof(struct sockaddr_in);

	printf("Starting server sync\n");	//debug

	number_files_client = get_dir_file_info(user.folder, localFiles);
	sprintf(packet.buffer, "%d", number_files_client);
	packet.ack = FALSE;

	/* Getting an ACK */
	while (packet.ack == FALSE) {

		/* Sends the number of files to server */
		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &to, &length); 
		if (status < 0) {
			printf("ERROR reading from socket in sync-client remote\n");
		}
	}
	packet.ack = FALSE;
	for(int i = 0; i < number_files_client; i++) {

		strcpy(buffer, localFiles[i].name);
		printf("Name sent: %s\n", localFiles[i].name);	//debug

		do { /* ACK */
	
			/* Sends file's name to server */
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			packet.ack = TRUE;
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &to, &length);
		}while(packet.ack != TRUE || (strcmp(packet.user, SERVER_USER) != 0));

		if (status < 0) {
			printf("ERROR writing to socket\n");
		}

		strcpy(buffer, localFiles[i].last_modified);
		printf("Last modified: %s\n", localFiles[i].last_modified);	//debug
		packet.ack = FALSE;

		do { /* ACK */
	
			/* Sends last modified to server */
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
			packet.ack = TRUE;
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &to, &length);
		}while(packet.ack != TRUE || (strcmp(packet.user, SERVER_USER) != 0));

		if (status < 0) {
			printf("ERROR writing to socket on sync client\n");	//debug
		}
		
		packet.ack = FALSE;
		do { /* ACK */
	
			/* Reads from server */
			status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &from, &length);
			
			packet.ack = TRUE;
			status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
		
		}while(packet.ack != TRUE);

		if (status < 0) {
			printf("ERROR reading from socket\n");
		}
		printf("Received: %s\n", buffer);	//debug

		if(strcmp(buffer, S_GET) == 0) {
			sprintf(path, "%s/%s", user.folder, localFiles[i].name);
			//send_file(path, FALSE);					//interface implementation
			printf("Sending file %s\n", path);	//debug
		}
	}

	printf("End of server sync\n");		//debug
}

/* Uses inotify to watch sync time in a certain period of time */
void *watcher(void* ptr_path) {

	char* watch_path = malloc(strlen((char*) ptr_path));
	strcpy(watch_path, (char*) ptr_path);

	int fd, wd;
	int length, i = 0;
	char buffer[EVENT_BUF_LEN];

	fd = inotify_init();
	if(fd < 0) {
		printf("Error inotify_init\n");		
	}
	
	wd = inotify_add_watch(fd, watch_path, IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO);

	char path[MAXNAME];
	int thread_running = TRUE;

	while(thread_running) {
		length = read(fd, buffer, EVENT_BUF_LEN); 

		if (length < 0) {
			thread_running = FALSE;
	    	} else {
	      		i = 0;
	     	 	while (i < length) {
				struct inotify_event* event = (struct inotify_event *) &buffer[i];

				if (event->len) {
		  			sprintf(path, "%s/%s", watch_path, event->name);

		  			if (event->mask & (IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO)) {
		    				if (check_dir(path) && (event->name[0] != '.')) {
		      					printf("\nRequest upload: %s\n", path);
		      					//send_file(path, FALSE);		interface implementation
		    				}
		  			} else if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
	    					if (event->name[0] != '.') {
	      						printf("\nRequest delete: %s\n", path);
	      						//delete_file(path);			interface implementation
	    					}
		  			}
				}

				i += EVENT_SIZE + event->len;
	      		}
		}

		usleep(100);
	}

	inotify_rm_watch(fd, wd);
	close(fd);

	return SUCCESS;
}
