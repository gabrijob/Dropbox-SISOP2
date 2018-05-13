#ifndef SERVER_CODE
#define SERVER_CODE

#include "dropboxServer.h"

/*   Global variables   */
ClientList clients_list; 
ServerInfo serverInfo;
sem_t semaphore;


void receive_file(char* filename, int sockid, int id) {
	char filepath[3*MAXNAME];
	int bytes_received;
	int file_size;
	int func_return;
	
	Frame packet;
	struct sockaddr_in cli_addr;		
	socklen_t clilen = sizeof(struct sockaddr_in);

	sprintf(filepath, "%s/%d/%s", serverInfo.folder, id, filename);
	printf("Receiving file at %s", filepath); //DEBUG
	FILE* file;
	file = fopen(filepath, "wb");

	if(file) {
		/* Receives the file size from client*/
		func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (func_return < 0) 
			printf("ERROR on recvfrom\n");
		
		file_size = atoi(packet.buffer);
		if(file_size == 0) {
			fclose(file);
			return;
		}

		bzero(packet.buffer, BUFFER_SIZE -1);		
		packet.ack = TRUE;

		/* Receives the file in BUFFER_SIZE sized parts*/
		bytes_received = 0;
		while(file_size > bytes_received) {
			func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (func_return < 0) 
				printf("ERROR on recvfrom\n");

			if((file_size - bytes_received) > BUFFER_SIZE) {
				fwrite(packet.buffer, sizeof(char), BUFFER_SIZE, file);
				bytes_received += sizeof(char) * BUFFER_SIZE; 
			}
			else {
				fwrite(packet.buffer, sizeof(char), (file_size - bytes_received), file);
				bytes_received += sizeof(char) * (file_size - bytes_received);
			}
			printf("\n Receiving file %s - Total: %d / Written: %d", filename, file_size, bytes_received); //DEBUG
		} 
		printf("\n Finished receiving file %s", filename); //DEBUG
		fclose(file);
	}
	else
		printf("Erro ao abrir o arquivo %s", filepath);
}


void send_file(char *filename, int sockid, int id, struct sockaddr_in *cli_addr) {
	char filepath[3*MAXNAME];
	int bytes_sent;
	int file_size;
	int func_return;

	Frame packet;

	sprintf(filepath, "%s/%d/%s", serverInfo.folder, id, filename);
	printf("Sending file at %s", filepath); //DEBUG

	FILE* file;
	file = fopen(filepath, "rb");
	
	if(file) {
		file_size = getFileSize(file);
		if(file_size == 0) {
			fclose(file);
			return;
		}

		sprintf(packet.buffer, "%d", file_size);
		//packet.ack == FALSE;
		/* Sends the file size to the client*/
		func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
		if (func_return < 0) 
			printf("ERROR on sendto\n");


		/* Sends the file in BUFFER_SIZE sized parts*/
		bytes_sent = 0;
		while(!feof(file)) {
			fread(packet.buffer, sizeof(char), BUFFER_SIZE, file);
			bytes_sent += sizeof(char) * BUFFER_SIZE;

			func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
			if (func_return < 0) 
				printf("ERROR on sendto\n");

			printf("\n Sending file %s - Total: %d / Read: %d", filename, file_size, bytes_sent); //DEBUG
		}
		printf("\n Finished sending file %s", filename);
		fclose(file);
	}
	else
		printf("Erro ao abrir o arquivo %s", filepath);
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

int new_server_port(char *address, Connection* connection) {
	struct sockaddr_in server;
	int sockid;
	int new_port = DYN_PORT_START;
	
  	/* Creating the socket */
	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 	if (sockid == ERROR) {
		printf("Error opening socket\n");
		return ERROR;
	}

	/* Get new server based on the new port*/
	bzero((char *) &server, sizeof(server));
	server.sin_family = AF_INET; 
	server.sin_port = htons(new_port); 
	server.sin_addr.s_addr = inet_addr(address);
	
	/* Keeps trying to bind the socket to a port until it finds one available */
  	while (bind(sockid, (struct sockaddr *) &server, sizeof(server)) == ERROR) { 
		new_port++;
		server.sin_port = htons(new_port);
		if(new_port > DYN_PORT_END) {
			perror("Falha na nomeação do socket\n");
			return ERROR;
		}
  	}

	printf("\nNew port: %d & New socket: %d\n", new_port, sockid); //DEGBUG
	connection->socket_id = sockid;
	connection->port = new_port;

	return SUCCESS;	
}

void select_commands(Frame* packet, struct sockaddr_in *cli_addr, int socket) {
	int func_return;
	socklen_t clilen = sizeof(struct sockaddr_in);
	packet->ack = TRUE;

	/* UPLOAD */
	if(strcmp(packet->buffer, UP_REQ) == 0) {
		strcpy(packet->buffer, F_NAME_REQ);
		/* Request filename */
		func_return = sendto(socket, packet, sizeof(*packet), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
		if (func_return < 0) 
			printf("ERROR on sendto\n");

		func_return = recvfrom(socket, packet, sizeof(*packet), 0, (struct sockaddr *) cli_addr, &clilen);
		if (func_return < 0) 
			printf("ERROR on recvfrom\n");
					
		char filename[MAXNAME];
		sprintf(filename, "%s", packet->buffer);
		receive_file(filename, socket, atoi(packet->user));	
	} 
	/* DOWNLOAD */
	else if(strcmp(packet->buffer, DOWN_REQ) == 0) {
		strcpy(packet->buffer, F_NAME_REQ);
		/* Request filename */
		func_return = sendto(socket, packet, sizeof(*packet), 0,(struct sockaddr *) cli_addr, sizeof(struct sockaddr));
		if (func_return < 0) 
			printf("ERROR on sendto\n");

		func_return = recvfrom(socket, packet, sizeof(*packet), 0, (struct sockaddr *) cli_addr, &clilen);
		if (func_return < 0) 
			printf("ERROR on recvfrom\n");

		char filename[MAXNAME];
		sprintf(filename, "%s", packet->buffer);
		send_file(filename, socket, atoi(packet->user), cli_addr);		
	}

}

void* clientThread(void* connection_struct) {

	puts("Reached control thread");
	struct sockaddr_in cli_addr;		
	socklen_t clilen = sizeof(struct sockaddr_in);
	int func_return;

	int socket;
	char client_id[MAXNAME];
	char *client_ip;
	
	Connection *connection = (Connection*) malloc(sizeof(Connection));
	Client *client = (Client*) malloc(sizeof(Client));
	
	/* Assignments */
	
	connection = (Connection*) connection_struct;
	socket = connection->socket_id;
	client_ip = connection->ip;
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
		newDevice(client, socket);
	}

	if(client->devices[0] > -1 || client->devices[1] > -1) {
		char client_folder[5*MAXNAME];

		sprintf(client_folder, "%s/%s", serverInfo.folder, client_id);
		if(check_dir(client_folder) == FALSE) {
			if(mkdir(client_folder, 0777) != SUCCESS) {
				printf("Error creating client folder '%s'.\n", client_folder);
				return NULL;
			}
		}
		/* starts sync */
		sync_server(socket, client, serverInfo); //-> NOT TESTED YET

		int connected = TRUE;
		Frame packet;

		/* Waits for commands */
		while(connected == TRUE) {
			printf("\nWaiting for commands from client-%s at port-%d/socket-%d\n", client_id, connection->port, socket); //DEBUG
			bzero(packet.buffer, BUFFER_SIZE -1);
			func_return = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (func_return < 0) 
				printf("ERROR on recvfrom\n");
			
			if(strcmp(packet.buffer, END_REQ) == 0) {
				connected = FALSE;
				sem_post(&semaphore);
			}
			else {
				//printf("\nComando recebido: %s", packet.buffer);
				select_commands(&packet, &cli_addr, socket);
			}
		}
	}
	
	return 0;
}


/* Connects server to clients */
void wait_connection(char* address, int sockid) {

	int  func_return;
	char *client_ip;

	Frame packet_server, packet;
		 
	socklen_t clilen;

	/* Identifier for the thread that will controll maixmum accesses */
	pthread_t thread_id;

	struct sockaddr_in cli_addr;
		
	clilen = sizeof(struct sockaddr_in);

	packet_server.ack = FALSE; //controls the acks received by the server
	while (TRUE) {
		
		//while(packet_server.ack != TRUE){
			
			func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (func_return < 0) 
				printf("ERROR on recvfrom\n");
			printf("Received a datagram from: %s\n", packet.user);
			//printf("ACK 1 SERVER = %d\n", packet_server.ack); DEBUG
		
			bzero(packet.buffer, BUFFER_SIZE -1);		
			strcpy(packet.buffer, "Got yout message\n");
			strcpy(packet_server.user, SERVER_USER);
			packet.ack = TRUE; packet_server.ack = TRUE; 
			/*
			func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
			if (func_return < 0) 
				printf("ERROR on sendto\n");
			*/
			//printf("ACK 2 SERVER = %d\n", packet_server.ack); DEBUG

		//}packet_server.ack = FALSE;

		/* Updates semaphore when a new connection starts */		
		sem_wait(&semaphore);
		
		/* inet_ntoa converts the network address into a string */
      		client_ip = inet_ntoa(cli_addr.sin_addr); 

		/* Starts a new client connection */
		Connection *connection = malloc(sizeof(*connection));
		
		if (new_server_port(address, connection) == SUCCESS) {
			connection->ip = client_ip;
			/* Field buffer is used to get information from client */
			strcpy(connection->client_id, packet.user);

			/* Creates thread to control clients access to server */
			if(pthread_create(&thread_id, NULL, clientThread, (void*) connection) < 0)
				printf("Error on creating thread\n");

			/*Sends new port to client*/
			sprintf(packet.buffer, "%d", connection->port);
			strcpy(connection->buffer, packet.buffer);
			func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
			if (func_return < 0) 
				printf("ERROR on sendto\n");
		}
		else printf("\nError creating new connection for client\n");	
	}
}


int main(int argc, char *argv[]) {

	int port, sockid;
	char *address;

	/* Initializing semaphore to admite a defined number of users */
	sem_init(&semaphore, 0, MAX_CLIENTS);

	if (argc > 3) {
		puts("Invalid number of arguments!");
		puts("Expected: './dropboxClient address port' or only './dropboxClient'");
		puts("In the second case the values of address and port will be set to default (see headers)");

		return ERROR;
	}

	address = malloc(strlen(DEFAULT_ADDRESS));	

	/* Parsig entries */
	if (argc == 3) {
		if (strlen(argv[1]) != strlen(DEFAULT_ADDRESS)) {
			free(address);
			address = malloc(strlen(argv[1]));
		}  
		strcpy(address, argv[1]);
		port = atoi(argv[2]);
			
	} else {
		strcpy(address, DEFAULT_ADDRESS);
		port = DEFAULT_PORT;
	}
	/* End of parsing */

	/* Creating socket and binding server */
	sockid = open_server(address, port);
	if (sockid != ERROR) {
		printf("\n-----Server information-----\nAddress: %s\nMain port: %d\nMain socket: %d\n", address, port, sockid);
		/* Checking if the server dir exists
			-> if not, creates it using mkdir with 0777 (full access) permission
			-> uses 'sys/stat.h' lib */
		if(check_dir(serverInfo.folder) == FALSE) {
			if(mkdir(serverInfo.folder, 0777) != SUCCESS) {
				printf("Error creating server folder '%s'.\n", serverInfo.folder);
				return ERROR;
			}
		}
		
		wait_connection(address, sockid);
	}

	return 0;
}

#endif
