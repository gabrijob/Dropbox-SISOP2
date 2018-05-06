#ifndef SERVER_CODE
#define SERVER_CODE

#include "dropboxServer.h"

/*   Global variables   */
ClientList clients_list; 
ServerInfo serverInfo;
sem_t semaphore;


/*void sync_server() {


}*/

void receive_file(char* filename, int sockid) {
	int bytes_written;
	int func_return;
	int file_size;

	struct sockaddr_in cli_addr;		
	socklen_t clilen = sizeof(struct sockaddr_in);

	FILE* file;
	char buffer[BUFFER_SIZE];

	file = fopen(filename, "wb");
	if(file) {
		/* Receives the file size from client*/
		func_return = recvfrom(sockid, &buffer, sizeof(buffer), 0, (struct sockaddr *) &cli_addr, &clilen);
		if (func_return < 0) 
			printf("ERROR on recvfrom\n");
		file_size = atoi(buffer);

		/* Receives the file in BUFFER_SIZE sized parts*/
		bytes_written = 0;
		while(file_size > bytes_written) {
			func_return = recvfrom(sockid, &buffer, sizeof(buffer), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (func_return < 0) 
				printf("ERROR on recvfrom\n");

			if((file_size = bytes_written) > BUFFER_SIZE) {
				fwrite(buffer, sizeof(char), BUFFER_SIZE, file);
				bytes_written += sizeof(char) * BUFFER_SIZE; 
			}
			else {
				fwrite(buffer, sizeof(char), (file_size = bytes_written), file);
				bytes_written += sizeof(char) * (file_size = bytes_written);
			}
			printf("\n Receiving file %s - Total: %d / Written: %d", filename, file_size, bytes_written); //DEBUG
		} 
		printf("\n Finished receiving file %s", filename); //DEBUG
		fclose(file);
	}
	else
		printf("Erro ao abrir o arquivo %s", filename);
}
/*
void send_file(char *file, int sockid) {


}*/

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

void* clientThread(void* connection_struct) {

	puts("Reached control thread");
	struct sockaddr_in cli_addr;		
	socklen_t clilen = sizeof(struct sockaddr_in);
	int func_return;

	int socket;
	char client_id[MAXNAME];
	
	Connection *connection = (Connection*) malloc(sizeof(Connection));
	Client *client = (Client*) malloc(sizeof(Client));
	
	/* Assignments */
	
	connection = (Connection*) connection_struct;
	socket = connection->socket_id;
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

		//sync_server()

		int connected = TRUE;
		char buffer[BUFFER_SIZE];
		bzero(buffer, BUFFER_SIZE);

		/* Waits for commands */
		while(connected == TRUE) {
			func_return = recvfrom(socket, &buffer, sizeof(buffer), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (func_return < 0) 
				printf("ERROR on recvfrom\n");
			
			if(strcmp(buffer, END_REQ) == 0) {
				connected = FALSE;
				sem_post(&semaphore);
			}
			else {
				//select_commands()
			}
		}
	}
	
	return 0;
}


/* Connects server to clients */
void wait_connection(int sockid) {

	int  func_return, new_client_socket;
	char *client_ip;

	Frame packet_server, packet;
		 
	socklen_t clilen;

	/* Identifier for the thread that will controll maixmum accesses */
	pthread_t thread_id;

	struct sockaddr_in cli_addr;
		
	clilen = sizeof(struct sockaddr_in);

	packet_server.ack = FALSE; //controls the acks received by the server
	while (TRUE) {
		
		while(packet_server.ack != TRUE){
			
			func_return = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) &cli_addr, &clilen);
			if (func_return < 0) 
				printf("ERROR on recvfrom\n");
			printf("Received a datagram from: %s\n", packet.user);
			//printf("ACK 1 SERVER = %d\n", packet_server.ack); DEBUG
		
			bzero(packet.buffer, BUFFER_SIZE -1);		
			strcpy(packet.buffer, "Got yout message\n");
			packet.ack = TRUE; packet_server.ack = TRUE;

			func_return = sendto(sockid, &packet, sizeof(packet), 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
			if (func_return < 0) 
				printf("ERROR on sendto\n");
			//printf("ACK 2 SERVER = %d\n", packet_server.ack); DEBUG

		}packet_server.ack = FALSE;

		/* Updates semaphore when a new connection starts */		
		sem_wait(&semaphore);
		
		/* inet_ntoa converts the network address into a string */
      		client_ip = inet_ntoa(cli_addr.sin_addr); 

		/* Starts a new client connection */
		Connection *connection = malloc(sizeof(*connection));

		new_client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 		if (new_client_socket == ERROR) 
			printf("Error on attributing new socket\n");

		/* For debug */
		//else
			//printf("Socket of the new user %i\n", new_client_socket);

		connection->socket_id = new_client_socket;
		connection->ip = client_ip;
		/* Field buffer is used to get information from client */
		strcpy(connection->buffer, packet.buffer);
		strcpy(connection->client_id, packet.user);

		/* Creates thread to control clients access to server */
		if(pthread_create(&thread_id, NULL, clientThread, (void*) connection) < 0)
			printf("Error on creating thread\n");	
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
		
		/* Checking if the server dir exists
			-> if not, creates it using mkdir with 0777 (full access) permission
			-> uses 'sys/stat.h' lib */

		if(check_dir(serverInfo.folder) == FALSE) {
			if(mkdir(serverInfo.folder, 0777) != SUCCESS) {
				printf("Error creating server folder '%s'.\n", serverInfo.folder);
				return ERROR;
			}
		}
		
		wait_connection(sockid);
	}

	return 0;
}

#endif
