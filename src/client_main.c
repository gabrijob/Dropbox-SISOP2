#ifndef CLIENT_MAIN_CODE
#define CLIENT_MAIN_CODE

#include "dropboxClient.h"

/*   Global variables   */
UserInfo user;
MSG_ID msg_id;
pthread_t local_sync_thread;
pthread_t remote_sync_thread;


void list_server() {

	char buffer[BUFFER_SIZE];
	int sockid = user.socket_id;

	struct sockaddr_in *serv_conn = user.serv_conn;

	int number_files = 0;

	printf("\n-SERVER DIRECTORY CONTENT-\n");

	/* Request List Server */
	strcpy(buffer, LIST_S_REQ);
	if(send_packet(&msg_id.client, buffer, sockid, serv_conn) < 0) {
		printf("ERROR sending list server request\n");
		return;
	}


	/* Receive ack and number of files from server*/
	struct sockaddr_in from;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&msg_id.server, buffer, sockid, &from) < 0) {
		printf("ERROR receiving number of files at server\n");
		return;
	}

	number_files = atoi(buffer);
	printf("\nNumber of files: %d\n", number_files);
	for(int i = 0; i < number_files; i++) {

		bzero(buffer, BUFFER_SIZE -1);
		if(recv_packet(&msg_id.server, buffer, sockid, &from) < 0) {
			printf("ERROR receiving file data from server\n");
			return;
		}
		printf("%s\n", buffer);
	}
}


void list_client() {
	if(!fileExists(user.folder)) {
    	printf("Error, User folder '%s' doesn't exist.\n", user.folder);
  	} else {
		printf("\n-CLIENT DIRECTORY CONTENT-\n");
    	print_dir_file_info(user.folder);
  	}
}

void get_sync_dir() {
	int sockid = user.socket_id;
	struct sockaddr_in *serv_conn = user.serv_conn;

	char buffer[BUFFER_SIZE];

	/* Send synchronization request to server */
	strcpy(buffer, SYNC_REQ);
	if(send_packet(&msg_id.client, buffer, sockid, serv_conn) < 0)
		printf("\nERROR sending synchronization request");

	sync_client(&user, &msg_id);
}


void client_menu() {
	char command_line[MAXPATH];
	char *command;
	char *attribute;
	char *attribute_download;
	int r_sync_tid;
	int l_sync_tid;

	/* cria threads para manter a sincronização local */
	if((l_sync_tid = pthread_create(&local_sync_thread, NULL, answer_pending, (void *) &user))) {
		printf("Syncronization Thread 2 creation failed: %d\n", l_sync_tid);
	}

	/* cria threads para manter a sincronização remota */
	if((r_sync_tid = pthread_create(&remote_sync_thread, NULL, watcher, (void *) &user))) {
		printf("Syncronization Thread creation failed: %d\n", r_sync_tid);
	}
	

	int exited = FALSE;
	while(!exited){
		
		printf("\nEsperando comandos...\n");

		if(fgets(command_line, sizeof(command_line), stdin) != NULL) {
			command_line[strcspn(command_line, "\r\n")] = 0;

			if (strcmp(command_line, "exit") == 0) 
				exited = TRUE;
			else {
				command = strtok(command_line, " ");
				attribute = strtok(NULL, " ");
				attribute_download = strtok(NULL, " ");
			}

			/* UPLOAD */
			if(strcmp(command, "upload") == 0) {
				pthread_mutex_lock(&user.lock_server_comm);
				send_file_client(attribute, &user, &msg_id);
				pthread_mutex_unlock(&user.lock_server_comm);
			}
			/* DOWNLOAD */
			else if(strcmp(command, "download") == 0) {
				pthread_mutex_lock(&user.lock_server_comm);
				get_file(attribute, &user, attribute_download, &msg_id);
				pthread_mutex_unlock(&user.lock_server_comm);
			}
			/* LIST_SERVER */
			else if(strcmp(command, "list_server") == 0) {
				pthread_mutex_lock(&user.lock_server_comm);
				list_server();
				pthread_mutex_unlock(&user.lock_server_comm);
			}
			/* LIST_CLIENT */
			else if(strcmp(command, "list_client") == 0) {
				list_client();;
			}
			/* GET_SYNC_DIR*/
			else if(strcmp(command, "get_sync_dir") == 0) {
				pthread_mutex_lock(&user.lock_server_comm);
				get_sync_dir();
				pthread_mutex_unlock(&user.lock_server_comm);
			} 
			/* DELETE */
			else if(strcmp(command, "delete") == 0) {
				pthread_mutex_lock(&user.lock_server_comm);
				delete_file(attribute, &user, &msg_id);
				pthread_mutex_unlock(&user.lock_server_comm);
			}
			/* INVALID COMMAND*/
			/*else {
			}*/	
		}
		else
			printf("\nFalha ao ler comando");
	}
    //Fecha a thread de sincronizacao
	pthread_cancel(local_sync_thread);
	pthread_cancel(remote_sync_thread);
	printf("\nSync threads killed");  //debug

	close_session(&user, &msg_id);
}


int main(int argc, char *argv[]) {

	int port, sockid;
	char *address;


	if (argc != 4) {
		puts("Error! Insuficient Arguments");
		puts("Expected: './dropboxClient user address port'");

		return ERROR;
	}

	/* Setting user information by parsing entries */
	if (strlen(argv[1]) <= MAXNAME) {
		if (strcmp(argv[1], SERVER_USER) == 0) {
			printf("Invalid User! Please do not use 'server' as ID...\n");
			return ERROR;
		}

		strcpy(user.id, argv[1]);
		sprintf(user.folder, "%s/sync_dir_%s", getUserHome(), user.id);
	} else {
		puts("Maximum user ID size exceeded\n");
		printf("Maximum: %d\n", MAXNAME);

		return ERROR;
	}
	
	address = malloc(strlen(argv[2]));
	strcpy(address, argv[2]);

	port = atoi(argv[3]);
	/* End of initial parsing */


	/* Starts communication with the server
	        -> Opens a socket UDP */
	sockid = login_server(address, port, &user, &msg_id); 

	if (sockid == SUCCESS) {	
		/* Display client interface */
		client_menu();

	} else {
		printf("Could not connect to server '%s' at port '%d'\n", address, port);
		return ERROR;
	}
}

#endif
