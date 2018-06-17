#ifndef SERVER_MAIN_CODE
#define SERVER_MAIN_CODE

#include "dropboxServer.h"
#include "server_replication.h"


/*   Global variables   */
ClientList clients_list;
ServerInfo serverInfo;
sem_t semaphore;



void list_server(int sockid, Client* client, struct sockaddr_in *cli_addr, MSG_ID *msg_id) {
	char client_folder[3*MAXNAME];
    char buffer[BUFFER_SIZE];

	sprintf(client_folder, "%s/%s", serverInfo.folder, client->userid);

	/* Update files list */
	client->n_files = get_dir_file_info(client_folder, client->files);

	sprintf(buffer, "%d", client->n_files);
	printf("\nGot request to list user-%s files", client->userid);
	printf("\nNumber files: %d\n", atoi(buffer)); // debug

	/* Send number of files to client */
    if(send_packet(&msg_id->server, buffer, sockid, cli_addr) < 0) {
        printf("\nERROR sending number of files to client");
    }

	/* Send files metadata */
	for(int i = 0; i < client->n_files; i++) {
		sprintf(buffer, "%s \t- Modification Time: %s", client->files[i].name, client->files[i].last_modified);
		
        if(send_packet(&msg_id->server, buffer, sockid, cli_addr) < 0) {
            printf("\nERROR sending file data to client");
        }
    }
}

int remove_file(char* filename, Client *client) {
	char filepath[4*MAXNAME];
	int ret;

	char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s", serverInfo.folder, client->userid);

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

	client->n_files = get_dir_file_info(client_folder, client->files);

	return ret;
}	


int open_server(char *address, int port) {

	struct sockaddr_in server;
	int sockid;

	/* Initialize socket and server structures */
  	bzero((char *) &server, sizeof(server));

	server.sin_family = AF_INET; 
	server.sin_port = htons(port); 
	server.sin_addr.s_addr = inet_addr(address);

	sprintf(serverInfo.folder, "%s/%s", getUserHome(), SERVER_FOLDER);
	strcpy(serverInfo.ip, address);
	serverInfo.port = port;
	/* End of initialization */


  	/* Creating the socket */
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 	if (sockid == ERROR) {
		printf("Error opening socket\n");
		return ERROR;
	}
	
	/* BIND */
  	if(bind(sockid, (struct sockaddr *) &server, sizeof(server)) == ERROR) { 
    		perror("Falha na nomeação do socket\n");
    		return ERROR;
  	}
	
	return sockid;	
}


void select_commands(char *buffer, struct sockaddr_in *cli_addr, int socket, Client* client, MSG_ID* msg_id) {
	char client_folder[3*MAXNAME];
	sprintf(client_folder, "%s/%s", serverInfo.folder, client->userid);
	int device_index = getDevice(client, socket);

	/* UPLOAD */
	if(strcmp(buffer, UP_REQ) == 0 || strcmp(buffer, UP_REQ_S) == 0) {
		int update_client = TRUE;
		if (strcmp(buffer, UP_REQ_S) == 0) update_client = FALSE;

		/* Request filename */
		strcpy(buffer, F_NAME_REQ);
        if(send_packet(&msg_id->server, buffer, socket, cli_addr) < 0)
        	printf("\nERROR requesting file name");
        
        /* Receive filename */
        if(recv_packet(&msg_id->client, buffer, socket, cli_addr) < 0)
        	printf("\nERROR receiving file name");

		char filename[MAXNAME];
		sprintf(filename, "%s", buffer);
		int file_index = getFileIndex(filename, client->files, client->n_files);
		receive_file(filename, socket, client->userid, msg_id, &(client->mutex_files[file_index]));	

		
		/* Update files list */
		client->n_files = get_dir_file_info(client_folder, client->files);
		/* Marks that there are pending changes */
		client->pending_changes[0] = TRUE;
		client->pending_changes[1] = TRUE;
		if(update_client == FALSE) client->pending_changes[device_index] = FALSE;		
	}
	/* DOWNLOAD */
	else if(strcmp(buffer, DOWN_REQ) == 0) {
		strcpy(buffer, F_NAME_REQ);
		/* Request filename */
        if(send_packet(&msg_id->server, buffer, socket, cli_addr) < 0)
        	printf("\nERROR requesting file name");
        
        /* Receive filename */
        if(recv_packet(&msg_id->client, buffer, socket, cli_addr) < 0)
        	printf("\nERROR receiving file name");
        
		char filename[MAXNAME];
		sprintf(filename, "%s", buffer);
		int file_index = getFileIndex(filename, client->files, client->n_files);
		send_file(filename, socket, client->userid, cli_addr, msg_id, &(client->mutex_files[file_index]));		
	}
	/* LIST_SERVER */
	else if(strcmp(buffer, LIST_S_REQ) == 0) {
		list_server(socket, client, cli_addr, msg_id);
	}
	/* GET_SYNC_DIR */
	else if(strcmp(buffer, SYNC_REQ) == 0) {
		sync_server(socket, client, msg_id);
	}
	/* DELETE */
	else if(strcmp(buffer, DEL_REQ) == 0 || strcmp(buffer, DEL_REQ_S) == 0) {
		int update_client = TRUE;
		if (strcmp(buffer, DEL_REQ_S) == 0) update_client = FALSE;

		/* Request filename */
		strcpy(buffer, F_NAME_REQ);
        if(send_packet(&msg_id->server, buffer, socket, cli_addr) < 0)
        	printf("\nERROR requesting file name");

        /* Receive filename */
        if(recv_packet(&msg_id->client, buffer, socket, cli_addr) < 0)
        	printf("\nERROR receiving file name");

		char filename[MAXNAME];
		sprintf(filename, "%s",buffer);
		if(remove_file(filename, client) == 0)
			strcpy(buffer, DEL_COMPLETE);

		/* Send confirmation */
       	if(send_packet(&msg_id->server, buffer, socket, cli_addr) < 0)
        	printf("\nERROR sending deletion confirmation");

		/* Update files list */
		client->n_files = get_dir_file_info(client_folder, client->files);
		/* Marks that there are pending changes */
		client->pending_changes[0] = TRUE;
		client->pending_changes[1] = TRUE;
		if(update_client == FALSE) client->pending_changes[device_index] = FALSE;	
	}
	/* ANSWER CLIENT IF NEEDS TO SYNC */
	else if(strcmp(buffer, NEED_SYNC) == 0) {
		/* Synchronizes client if there are pending changes */
		if(client->pending_changes[device_index] == TRUE) {

			strcpy(buffer, SYNC_REQ);
			if(send_packet(&msg_id->server, buffer, socket, cli_addr) < 0)
				printf("\nERROR requesting sync from server");

			sync_server(socket, client, msg_id);
			client->pending_changes[device_index] = FALSE;
		}
		/* If there's not must tell client also */
		else{
			strcpy(buffer, SYNC_NREQ);
			if(send_packet(&msg_id->server, buffer, socket, cli_addr) < 0)
				printf("\nERROR telling client to not sync");
		}
	}	

}

void* clientThread(void* connection_struct) {

	puts("Reached control thread");
	struct sockaddr_in *cli_addr;		

    	char buffer[BUFFER_SIZE];
	int socket;
	char client_id[MAXNAME];
	//char *client_ip;
	int device = 0;

	
	Connection *connection = (Connection*) malloc(sizeof(Connection));
	Client *client = (Client*) malloc(sizeof(Client));
	
	/* Assignments */
	
	connection = (Connection*) connection_struct;
	socket = connection->socket_id;
	cli_addr = connection->client_address;
	//client_ip = connection->ip;
	strncpy(client_id, connection->client_id, MAXNAME);
	client_id[MAXNAME - 1] = '\0';

	/* Search for client in Clients List */
	client = searchClient(client_id, clients_list);
	/* End of assignments */
	
	if (client == NULL) {

		/* Creates a new node for the client and adds in the GLOBAL clients list;
		   Attributes a node to the new client */
		clients_list = addClient(client_id, socket, clients_list);
		client = searchClient(client_id, clients_list);

		//-------FOR DEBUG-------//
		printUserList(clients_list);


	} else {
		/* Adds a new device for the client */
		device = newDevice(client, socket);
	}

	if(device != -1) {
		char client_folder[5*MAXNAME];

		sprintf(client_folder, "%s/%s", serverInfo.folder, client_id);
		if(check_dir(client_folder) == FALSE) {
			if(mkdir(client_folder, 0777) != SUCCESS) {
				printf("Error creating client folder '%s'.\n", client_folder);
				return NULL;
			}
		}

		client->n_files = get_dir_file_info(client_folder, client->files);
		printClientFiles(client); // DEBUG


		MSG_ID msg_id;
		msg_id.server = START_MSG_COUNTER;
		msg_id.client = START_MSG_COUNTER;
		/* Synchronize client with server */
		sync_server(socket, client, &msg_id);
		client->pending_changes[getDevice(client, socket)] = FALSE;

		int connected = TRUE;
		/* Waits for commands */
		while(connected == TRUE) {
			/* Waits for commands */
			//printf("\nWaiting for commands from client-%s at port-%d/socket-%d\n", client_id, connection->port, socket); //DEBUG
			bzero(buffer, BUFFER_SIZE -1);				
            if(recv_packet(&msg_id.client, buffer, socket, cli_addr) < 0) {
            	printf("\nERROR receiving command");
            }
			if(strcmp(buffer, END_REQ) == 0) {
				printf("\nDisconnecting...");
				connected = FALSE;
				sem_post(&semaphore);
			}
			else {
				//printf("\nComando recebido: %s", buffer);
				select_commands(buffer, cli_addr, socket, client, &msg_id);
			}
		}

		printf("user-%s desconectou no dispositivo %d, socket-%d!\n", client_id, removeDevice(client, device), socket);
		clients_list = check_login_status(client, clients_list);
	}
	else {
		printf("Too many devices for same client!!\n");
		return NULL;
	}
	
	return 0;
}


/* Connects server to clients */
void wait_connection(char* address, int sockid) {

	char *client_ip;
	int zero;
		 
	/* Identifier for the thread that will controll maixmum accesses */
	pthread_t thread_id;

	struct sockaddr_in conn_addr;
		

	while (TRUE) {
		char buffer[BUFFER_SIZE];

		/* Receive type of connection */
		bzero(buffer, BUFFER_SIZE -1);
		zero = START_MSG_COUNTER;
		if(recv_packet(&zero, buffer, sockid, &conn_addr) == 0)
			printf("Received a datagram: %s", buffer);

		if(strcmp(buffer, SVR_COM_TYPE) == 0) {
			new_backup_sv_conn(address, sockid, &conn_addr);
		}
		else if(strcmp(buffer, CLI_COM_TYPE) == 0) {
			/* Receive client id */
			bzero(buffer, BUFFER_SIZE -1);
			zero = START_MSG_COUNTER;
			if(recv_packet(&zero, buffer, sockid, &conn_addr) == 0)
				printf("\tFrom client %s\n", buffer);

			/* Updates semaphore when a new connection starts */		
			sem_wait(&semaphore);
			printf("\nAQUI?1");
			/* inet_ntoa converts the network address into a string */
			client_ip = inet_ntoa(conn_addr.sin_addr); 

			printf("\nAQUI?");
			/* Starts a new client connection */
			Connection *connection = (Connection*) malloc(sizeof(Connection));
			connection->client_address = &conn_addr;
			printf("\nOU AQUI?");
			int new_sockid, new_port;
			if (new_server_port(address, &new_sockid, &new_port) == SUCCESS) {
				connection->socket_id = new_sockid;
				connection->port = new_port;
				connection->ip = client_ip;
				/* Field buffer is used to get information from client */
				strcpy(connection->client_id, buffer);

				/*Sends new port to client*/
				sprintf(buffer, "%d", connection->port);
				strcpy(connection->buffer, buffer);
				zero = START_MSG_COUNTER;
				if(send_packet(&zero, buffer, sockid, &conn_addr) < 0)
					printf("\nERROR sending new port to client");

				/* Creates thread to control clients access to server */
				if(pthread_create(&thread_id, NULL, clientThread, (void*) connection) < 0)
					printf("Error on creating thread\n");

			}
			else printf("\nError creating new connection for client\n");
		}	
	}
}


int main(int argc, char *argv[]) {

	int port, sockid, primary_server_port;
	char address[MAXNAME], process_server[MAXNAME], primary_server_address[MAXNAME];
	//pthread_t thread_server;

	/* Initializing semaphore to admite a defined number of users */
	sem_init(&semaphore, 0, MAX_CLIENTS);

	if (argc > 6) {
		puts("Invalid number of arguments!");
		puts("Expected: './dropboxClient address port primary/backup' or only './dropboxClient'");
		puts("In the second case the values of address and port will be set to default (see headers)");

		return ERROR;
	}

	
	/* Parsig entries */
	if (argc == 4) {
		sprintf(address, "%s", argv[1]);
		port = atoi(argv[2]);
		sprintf(process_server, "%s", argv[3]);
			
	} 
	else if(argc == 6) {
		sprintf(address, "%s", argv[1]);
		port = atoi(argv[2]);
		sprintf(primary_server_address, "%s", argv[3]);
		primary_server_port = atoi(argv[4]);
		sprintf(process_server, "%s", argv[5]);
	}
	else {
		strcpy(address, DEFAULT_ADDRESS);
		port = DEFAULT_PORT;
		strcpy(process_server, DEFAULT_SERVER);
	}
	/* End of parsing */
	


	/* Creating socket and binding server */
	sockid = open_server(address, port);
	if (sockid != ERROR) {
		printf("\n-----Server information-----\nAddress: %s\nMain port: %d\nMain socket: %d\n----------------------------\n", address, port, sockid);

		/* Checking if the server dir exists
			-> if not, creates it using mkdir with 0777 (full access) permission
			-> uses 'sys/stat.h' lib */
		if(check_dir(serverInfo.folder) == FALSE) {
			if(mkdir(serverInfo.folder, 0777) != SUCCESS) {
				printf("Error creating server folder '%s'.\n", serverInfo.folder);
				return ERROR;
			}
		}

		/* Runs server */
		if(strcmp(process_server, BACKUP_SERVER) == 0)
			run_backup(address, port, primary_server_address, primary_server_port);
		else if(strcmp(process_server, DEFAULT_SERVER) == 0)		
			wait_connection(address, sockid);
	}

	return 0;
}

#endif
