#ifndef SERVER_REPLICATION_CODE
#define SERVER_REPLICATION_CODE

#include "server_replication.h"


//----------------------GLOBAL VARIABLES-----------------------------------------------------------------------
int my_id = 0;
int last_new_id = 0;
s_Connection* serversList[MAXSERVERS]; // id do servidor é sua posição no array
int last_client_index = 0;
C_DATA* clientsList[MAX_CLIENTS];

int is_new_backup[MAXSERVERS];
int is_new_client[MAX_CLIENTS];

//-------------------------------------------------------------------------------------------------------------

int init_server_connection(int port, char* address, int *sockid ,struct sockaddr_in *sv_conn) {

	if(sockid != NULL) {	
		*sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (*sockid == ERROR) {
			printf("\nError opening socket ");
			return ERROR;
		}
	}

	if(sv_conn == NULL) {
		printf("\nERROR pointer null");
		return ERROR;
	}

	sv_conn->sin_family = AF_INET;
	sv_conn->sin_port = htons(port);
	sv_conn->sin_addr.s_addr = inet_addr(address);

	return SUCCESS;
}



//====================================FUNÇÕES  SERVIDOR PRIMARIO=================================================================
void init_server_structs(int sid, int port, char* address) {

	/* Insert primary server on servers list*/
	s_Connection *connection = (s_Connection*) malloc(sizeof(s_Connection));
	connection->sid = sid;
	connection->port = port;
	strcpy(connection->address, address);

	serversList[sid] = connection;

	/* Initialize list of flags for new backups and new clients*/
	for(int i = 0; i < MAXSERVERS; i++) {
		is_new_backup[i] = FALSE;
	}

	for(int i = 0; i < MAX_CLIENTS; i++) {
		is_new_client[i] = FALSE;
	}

}

/* Insert client data on the clients list */
void register_client_login(char *id, struct sockaddr_in *cli_addr) {
	C_DATA *client_data = (C_DATA*) malloc(sizeof(C_DATA));
	strcpy(client_data->id, id);
	client_data->sock_addr = cli_addr;

	clientsList[last_client_index] = client_data;
	last_client_index++;
}

/* Indicates to whom primary server must send the new backup server data */
void signal_new_backup() {
	for(int i = 0; i < last_new_id; i++) {
		if((serversList[i] != NULL) && (serversList[i]->sid != -1) && (i != my_id)) {
			is_new_backup[i] = TRUE;
		}
	}
}

void new_backup_sv_conn(char* my_address, int old_sockid) {
	int zero;
    char buffer[BUFFER_SIZE];

	struct sockaddr_in sv_addr;

	pthread_t thread_id;

	/* Starts a new client connection */
	s_Connection *connection = (s_Connection*) malloc(sizeof(s_Connection));

	/* Receive backup server ip */
	bzero(buffer, BUFFER_SIZE -1);
	zero = START_MSG_COUNTER;
	if(recv_packet(&zero, buffer, old_sockid, &sv_addr) == 0)
		printf("\tFrom client %s\n", buffer);   
    
	sprintf(connection->address, "%s", buffer);

	/* Receive backup server port */
	bzero(buffer, BUFFER_SIZE -1);
	zero = START_MSG_COUNTER;
	if(recv_packet(&zero, buffer, old_sockid, &sv_addr) == 0)
		printf("/%s\n", buffer);

	connection->port = atoi(buffer);

    int new_sockid, new_port;		
	if (new_server_port(my_address, &new_sockid, &new_port) == SUCCESS) {
		connection->socket = new_sockid;

		last_new_id++;
        connection->sid = last_new_id;
		serversList[last_new_id] = connection;
		signal_new_backup();

		/*Sends new port to backup server*/
		sprintf(buffer, "%d", new_port);
		zero = START_MSG_COUNTER;
		if(send_packet(&zero, buffer, new_sockid, &sv_addr) < 0)
			printf("\nERROR sending new port to backup server");

		/*Send backup server id*/
		sprintf(buffer, "%d", last_new_id);
		zero = START_MSG_COUNTER;
		if(send_packet(&zero, buffer, new_sockid, &sv_addr) < 0)
			printf("\nERROR sending new port to backup server");

		/* Creates thread to communicate with backups */
    	if(pthread_create(&thread_id, NULL, serverThread, (void*) connection) < 0)
			printf("Error on creating thread\n");

	}
	else printf("\nError creating new connection for backup server\n");
}

/**
 *  Envia toda a lista de servers conhecidos pro novo server backup criado
 *  Manda id, endereço(ip) e porta especificamente
 *  Inclusive do servidor primario e o de backup que está recebendo
**/
int send_servers_to_new(int sockid) {
	int zero, sid;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in backup_addr;

	/* Receive request to servers list*/
	bzero(buffer, BUFFER_SIZE -1);
	zero = START_MSG_COUNTER;
	if(recv_packet(&zero, buffer, sockid, &backup_addr) < 0) {
		printf("\nERROR receiving request to send servers list to backup server");
		return ERROR;
	}

	printf("\nSending known servers to new backup server...");

	/* Send list lenght */
	sprintf(buffer, "%d", last_new_id);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, sockid, &backup_addr) < 0) {
		printf("\nERROR sending list lenght to backup server");
		return ERROR;
	}

	for(int i = 0; i <= last_new_id; i++) {

		if(serversList[i] == NULL)
			sid = -1;
		else
			sid = serversList[i]->sid;

		/* Send sid */
		sprintf(buffer, "%d", sid);
		zero = START_MSG_COUNTER;
		if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
			printf("\nERROR sending sid to backup server");	

		if(sid != -1) {

			/* Send port */
			sprintf(buffer, "%d", serversList[i]->port);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending port to backup server");	

			/* Send address */
			strcpy(buffer, serversList[i]->address);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending address to backup server");	

		}
	}



	printf("Done\n");
	return SUCCESS;
}

/**
 * Envia toda a lista de clientes conhecidos pro novo server backup criado
 * Precisa decidir quais dados enviar exatamente
 * Talvez sicronizar as pastas de cada cliente entre servidor primario e o novo backup aqui
 **/
int send_clients_to_new(int sockid) {
	int zero;
	char buffer[BUFFER_SIZE];
	char client_id[MAXNAME];
	char* sock_addr;

	struct sockaddr_in backup_addr;

	/* Receive request to clients list*/
	bzero(buffer, BUFFER_SIZE -1);
	zero = START_MSG_COUNTER;
	if(recv_packet(&zero, buffer, sockid, &backup_addr) < 0) {
		printf("\nERROR receiving request to send clients list to backup server");
		return ERROR;
	}

	printf("\nSending client data to new backup server...");

	/* Send list lenght */
	sprintf(buffer, "%d", last_client_index);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, sockid, &backup_addr) < 0) {
		printf("\nERROR sending list lenght to backup server");
		return ERROR;
	}

	for(int i = 0; i <= last_client_index; i++) {
		if(clientsList[i] == NULL)
			strcpy(client_id, "-1");
		else
			strcpy(client_id, clientsList[i]->id);

		/* Send sid */
		strcpy(buffer, client_id);
		zero = START_MSG_COUNTER;
		if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
			printf("\nERROR sending client id to backup server");


		if(strcmp(client_id, "-1") != 0) {
			sock_addr = (char*) clientsList[i]->sock_addr;
			/* Send socket address */
			memcpy(buffer, sock_addr, BUFFER_SIZE);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending socket address to backup server");
		}
	}

	printf("Done\n");
	return SUCCESS;
}



int send_new_server_to_backup(int to_sid) {
	int zero;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in backup_addr;

	s_Connection *to_backup = serversList[to_sid];

	if(to_sid == -1 || to_backup == NULL) {
		printf("\nERROR backup server unavailable");
		return ERROR;
	}

	int to_socket = to_backup->socket;
	int to_port = to_backup->port;
	char to_address[MAXNAME];
	strcpy(to_address, to_backup->address);

	/* Create struct for server socket address */
	if(init_server_connection(to_port, to_address, NULL, &backup_addr) == ERROR) {
		printf("\nERROR initiating server connection");
		return ERROR;
	}

	s_Connection *new_backup = serversList[last_new_id];

	if(new_backup == NULL) {
		printf("\nERROR new backup server unavailable");
		return ERROR;
	}

	/* Send new backup server signal to backup server */
	sprintf(buffer, "%s", NS_SIGNAL);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending sid to backup server");	

	/* Send sid */
	sprintf(buffer, "%d", last_new_id);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending sid to backup server");	

	/* Send port */
	sprintf(buffer, "%d", new_backup->port);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending port to backup server");	

	/* Send address */
	strcpy(buffer, new_backup->address);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending address to backup server");

	
	is_new_backup[to_sid] = FALSE;

	return SUCCESS;
}

/**
 * Thread de comunicação do servidor primario com um servidor de backup 
 **/
void* serverThread(void* connection_struct) {

	s_Connection *connection = (s_Connection*) connection_struct;
	
	int sockid = connection->socket;
	int sid = connection->sid;

    send_servers_to_new(sockid);
	serversListPrint(); 			//debug
	send_clients_to_new(sockid);
	clientsListPrint();				//debug

	while(TRUE)  {
		if(is_new_backup[sid]) 
			send_new_server_to_backup(sid);

		//if(NEW_CLIENT) send_new_client_to_backup		
	}	


    return NULL;
}


//====================================FUNÇÕES SERVIDOR BACKUP=================================================================
void run_backup(int sockid, char* address, int port, char* primary_server_address, int primary_server_port) {
    int primary_sockid;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in primary_sv_conn, from;
		
    primary_sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (primary_sockid == ERROR) {
		printf("\nError opening socket ");
		return;
	}
	else
		printf("\nFirst backup server socket %i\n", primary_sockid);


	bzero((char *) &primary_sv_conn, sizeof(struct sockaddr_in));

	primary_sv_conn.sin_family = AF_INET;
	primary_sv_conn.sin_port = htons(primary_server_port);
	primary_sv_conn.sin_addr.s_addr = inet_addr(primary_server_address);

	printf("\nStarting connection with primary server at %s/%d", primary_server_address, primary_server_port);

	int zero = START_MSG_COUNTER;
	/* Sends first message to server */
	strcpy(buffer, SVR_COM_TYPE);
	if(send_packet(&zero, buffer, primary_sockid, &primary_sv_conn) < 0) {
		printf("\nERROR starting connection with primary server"); 
		return;
	}

	zero = START_MSG_COUNTER;
	/* Sends ip */
	sprintf(buffer, "%s", address);
	if(send_packet(&zero, buffer, primary_sockid, &primary_sv_conn) < 0) {
		printf("\nERROR starting connection with primary server"); 
		return;
	}

	zero = START_MSG_COUNTER;
	/* Sends port */
	sprintf(buffer, "%d", port);
	if(send_packet(&zero, buffer, primary_sockid, &primary_sv_conn) < 0) {
		printf("\nERROR starting connection with primary server"); 
		return;
	}

	zero = START_MSG_COUNTER;
	/* Receives new port for connection */
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, primary_sockid, &from) < 0) {
		printf("\nERROR starting connection with server"); 
		return;
	}

    printf("\nNEW PORT: %s\n", buffer);
	primary_sv_conn.sin_port = htons(atoi(buffer)); //updates default port with port received from the primary server

	zero = START_MSG_COUNTER;
	/* Receives id */
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, primary_sockid, &from) < 0) {
		printf("\nERROR starting connection with server"); 
		return;
	}

	printf("\nSERVER ID: %s\n", buffer);

	my_id = atoi(buffer);
	
	recv_servers_list(primary_sockid, &primary_sv_conn);
	recv_clients_list(primary_sockid, &primary_sv_conn);
	wait_contact(sockid);
}

/**
 * Recebe lista de servidores conhecidos do servidor primário
 * Recebe id, endereço(ip) e porta, e armazena na sua própria lista
 **/ 
int recv_servers_list(int sockid, struct sockaddr_in *prim_sv) {
	int s_list_size, sid;
	char buffer[BUFFER_SIZE];

	int zero = START_MSG_COUNTER;
	/* Request servers list from primary server*/
	strcpy(buffer, SL_REQ);
	if(send_packet(&zero, buffer, sockid, prim_sv) < 0) {
		printf("\nERROR requesting servers list from primary server"); 
		return ERROR;
	}

	printf("\nReceiving list of known servers...");

	zero = START_MSG_COUNTER;
	/* Receives servers list size */
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, prim_sv) < 0) {
		printf("\nERROR receiving size of servers list");
		return ERROR;
	} 
	
	s_list_size = atoi(buffer);
	last_new_id = s_list_size;

	for(int i = 0; i <= s_list_size; i ++) {

		/* Receives sid at index i */
		zero = START_MSG_COUNTER;
		bzero(buffer, BUFFER_SIZE -1);
		if(recv_packet(&zero, buffer, sockid, prim_sv) < 0)
			printf("\nERROR receiving sid"); 
		
		sid = atoi(buffer);

		s_Connection *s_conn = (s_Connection*) malloc(sizeof(s_Connection));

		s_conn->sid = sid;

		if(sid != -1) {

			/* Receive port */
			zero = START_MSG_COUNTER;
			bzero(buffer, BUFFER_SIZE -1);
			if(recv_packet(&zero, buffer, sockid, prim_sv) < 0)
				printf("\nERROR receiving port"); 

			s_conn->port = atoi(buffer);

			/* Receive address */
			zero = START_MSG_COUNTER;
			bzero(buffer, BUFFER_SIZE -1);
			if(recv_packet(&zero, buffer, sockid, prim_sv) < 0)
				printf("\nERROR receiving address");

			strcpy(s_conn->address, buffer);
		}

		serversList[i] = s_conn;
	}

	printf("Done\n");
	serversListPrint(); //debug

	return SUCCESS;
}


/**
 * Recebe lista de clientes conhecidos do servidor primário
 * Recebe dados e armazena na sua própria lista
 **/ 
int recv_clients_list(int sockid, struct sockaddr_in *prim_sv){
	int c_list_size;

	char buffer[BUFFER_SIZE];
	char client_folder[MAXPATH];

	int zero = START_MSG_COUNTER;
	/* Request clients list from primary server*/
	strcpy(buffer, CL_REQ);
	if(send_packet(&zero, buffer, sockid, prim_sv) < 0) {
		printf("\nERROR requesting clients list from primary server"); 
		return ERROR;
	}

	printf("\nReceiving clients data...");

	/* Receives clients list size */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, prim_sv) < 0) {
		printf("\nERROR receiving size of clients list");
		return ERROR;
	} 

	c_list_size = atoi(buffer);
	last_client_index = c_list_size;


	for(int i = 0; i <= c_list_size; i ++) {
		/* Receive client id */
		zero = START_MSG_COUNTER;
		bzero(buffer, BUFFER_SIZE -1);
		if(recv_packet(&zero, buffer, sockid, prim_sv) < 0)
			printf("\nERROR receiving client id");

		C_DATA* client_data = (C_DATA*) malloc(sizeof(C_DATA));
		strcpy(client_data->id, buffer);

		if(strcmp(client_data->id, "-1") != 0){
			/* Receive client socket address*/
			zero = START_MSG_COUNTER;
			bzero(buffer, BUFFER_SIZE -1);
			if(recv_packet(&zero, buffer, sockid, prim_sv) < 0)
				printf("\nERROR receiving client socket address");

			client_data->sock_addr = (struct sockaddr_in *) buffer;

			/* Create client directory if it doesn't exist */
			sprintf(client_folder, "%s/%s/%s",getUserHome(), SERVER_FOLDER, client_data->id);
			if(check_dir(client_folder) == FALSE) {
				if(mkdir(client_folder, 0777) != SUCCESS) {
					printf("Error creating client folder '%s'.\n", client_folder);
					return ERROR;
				}
			}

			clientsList[i] = client_data;

			/* Synchornize client directory with main server's */
		}

	}
	
	printf("Done\n");
	clientsListPrint(); //debug

	return SUCCESS;
}

/** 
 * Receive new backup server data and stores it on servers list 
 **/ 
int recv_new_server(int sockid) {
	int zero, sid;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in prim_sv;

	/* Receives sid  */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_sv) < 0)
		printf("\nERROR receiving sid"); 
		
	sid = atoi(buffer);

	if(sid > last_new_id)
		last_new_id = sid;

	s_Connection *s_conn = (s_Connection*) malloc(sizeof(s_Connection));

	s_conn->sid = sid;

	/* Receive port */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_sv) < 0)
		printf("\nERROR receiving port"); 

	s_conn->port = atoi(buffer);

	/* Receive address */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_sv) < 0)
		printf("\nERROR receiving address");

	strcpy(s_conn->address, buffer);


	serversList[sid] = s_conn;

	return SUCCESS;
}


/**
 * Espera comandos do servidor primário ou de outros backups
 **/ 
void wait_contact(int sockid) {
	int zero;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in conn_addr;


	while(TRUE) {

		/* Receive a order */
		bzero(buffer, BUFFER_SIZE -1);
		zero = START_MSG_COUNTER;
		if(recv_packet(&zero, buffer, sockid, &conn_addr) == 0)
			printf("\nReceived a datagram: %s", buffer);

		/* NEW BACKUP SERVER */
		if(strcmp(buffer, NS_SIGNAL) == 0) { 
			recv_new_server(sockid);
			serversListPrint(); 			//debug
		}

		//if(NB_SIGNAL) recv_new_backup
		//if(VOTE_SIGAL) start_election
		
		//if(UPLOAD) receive_file
		//if(DOWNLOAD) faz nada
		//if(DELETE) remove_file
	}

}

void start_election() {

	puts("election reached");

}

/*void* serverCom(void* server_struct) {
	
	struct sockaddr_in cli_addr, serv_conn;
	char buffer[BUFFER_SIZE];
	int sockid, new_sock, zero = 0;

	puts("server thread");

	s_Connection *connection = (s_Connection*) malloc(sizeof(s_Connection));
	connection = (s_Connection*) server_struct;
	
	/* SOCKET USED TO COMMUNICATE AS SERVER 
	sockid = connection->socket_id;
		printf("\nServer socket as client %i\n", sockid);

	printf("Infos: port-> %d, address-> %s\n", connection->port, connection->address);
	
	bzero((char *) &serv_conn, sizeof(serv_conn));
	serv_conn.sin_family = AF_INET;
	serv_conn.sin_port = htons(DEFAULT_PORT);
	serv_conn.sin_addr.s_addr = inet_addr(DEFAULT_ADDRESS);
	
	bzero(buffer, BUFFER_SIZE -1);

	if(strcmp(connection->address, DEFAULT_ADDRESS) == 0) {
		printf("primary\n");
		while(TRUE) {
			zero = 0;
			if(recv_packet(&zero, buffer, sockid, &cli_addr) == 0) 
				printf("Received a datagram from: %s\n", buffer);
			
			bzero(buffer, BUFFER_SIZE -1);
			strcpy(buffer, DEFAULT_ADDRESS);
			zero = 0;
			if(send_packet(&zero, buffer, sockid, &cli_addr) < 0) {
				printf("\nERROR starting coommunication with primary server\n"); 
				exit(1);
			}
		}
		
	}
	if(strcmp(connection->address, BACKUP2_ADDRESS) == 0) {
		printf("backup2\n");

		new_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	 	if (new_sock == ERROR) {
			printf("Error opening socket\n");
			exit(1);
		}	
		
		strcpy(buffer, BACKUP2_ADDRESS);
		zero = 0;
		if(send_packet(&zero, buffer, new_sock, &serv_conn) < 0) {
			printf("\nERROR starting coommunication with backup server 1"); 
			exit(1);
		}
		bzero(buffer, BUFFER_SIZE -1);
		zero = 0;
		if(recv_packet(&zero, buffer, new_sock, &cli_addr) == 0) 
			printf("Received a datagram from: %s\n", buffer);
		

	}
	if(strcmp(connection->address, BACKUP1_ADDRESS) == 0) {
		printf("backup1\n");

		new_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	 	if (new_sock == ERROR) {
			printf("Error opening socket\n");
			exit(1);
		}	
		
		strcpy(buffer, BACKUP1_ADDRESS);
		zero = 0;
		if(send_packet(&zero, buffer, new_sock, &serv_conn) < 0) {
			printf("\nERROR starting coommunication with backup server 1"); 
			exit(1);
		}
		bzero(buffer, BUFFER_SIZE -1);
		zero = 0;
		if(recv_packet(&zero, buffer, new_sock, &cli_addr) == 0) 
			printf("Received a datagram from: %s\n", buffer);
	}
		
	return 0;	
}*/


//====================================FUNÇÕES DEBUG=================================================================

void serversListPrint() {
	printf("\n-------------Known servers------------------");

	for(int i = 0; i <= last_new_id; i++) {
		if(serversList[i] == NULL)
			continue;

		printf("\nServer at position %d:", i);
		if(serversList[i]->sid == -1) {
			printf("\n\tN/A\n");
			continue;
		} 
		printf("\n\tSID: %d", serversList[i]->sid);
		printf("\n\tPort: %d", serversList[i]->port);
		printf("\n\tAddress: %s\n", serversList[i]->address);
	}
}

void clientsListPrint() {
	printf("\n-------------Clients data------------------");

	for(int i = 0; i <= last_client_index; i++) {
		if(clientsList[i] == NULL)
			continue;

		printf("\nClient at position %d:", i);
		if(strcmp(clientsList[i]->id, "-1") == 0) {
			printf("\n\tN/A\n");
			continue;
		} 
		printf("\n\tID: %s", clientsList[i]->id);

	}
}


#endif