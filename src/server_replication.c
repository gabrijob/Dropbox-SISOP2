#ifndef SERVER_REPLICATION_CODE
#define SERVER_REPLICATION_CODE

#include "server_replication.h"


int last_new_id = 0;
s_Connection serversList[MAXSERVERS]; // id do servidor é sua posição no array

//====================================FUNÇÕES  SERVIDOR PRIMARIO=================================================================
void new_backup_sv_conn(char* my_address, int old_sockid, struct sockaddr_in *sv_addr) {
	int zero;
    char buffer[BUFFER_SIZE];

	/* Starts a new client connection */
	s_Connection *connection = malloc(sizeof(s_Connection));

	/* Receive backup server ip */
	bzero(buffer, BUFFER_SIZE -1);
	zero = START_MSG_COUNTER;
	if(recv_packet(&zero, buffer, old_sockid, sv_addr) == 0)
		printf("\tFrom client %s\n", buffer);   
    
	sprintf(connection->address, "%s", buffer);

	/* Receive backup server port */
	bzero(buffer, BUFFER_SIZE -1);
	zero = START_MSG_COUNTER;
	if(recv_packet(&zero, buffer, old_sockid, sv_addr) == 0)
		printf("/%s\n", buffer);

	connection->port = atoi(buffer);

    int new_sockid, new_port;		
	if (new_server_port(my_address, &new_sockid, &new_port) == SUCCESS) {
		connection->socket = new_sockid;

		last_new_id++;
        connection->sid = last_new_id;

		/*Sends new port to backup server*/
		sprintf(buffer, "%d", new_port);
		zero = START_MSG_COUNTER;
		if(send_packet(&zero, buffer, new_sockid, sv_addr) < 0)
			printf("\nERROR sending new port to backup server");

		/*Send backup server id*/
		sprintf(buffer, "%d", last_new_id);
		zero = START_MSG_COUNTER;
		if(send_packet(&zero, buffer, new_sockid, sv_addr) < 0)
			printf("\nERROR sending new port to backup server");

		/* Creates thread to communicate with backups */
    	//if(pthread_create(&thread_id, NULL, serverThread, (void*) connection) < 0)
		//	printf("Error on creating thread\n");

	}
	else printf("\nError creating new connection for backup server\n");
}

/**
 *  Envia toda a lista de servers conhecidos pro novo server backup criado
 *  Manda id, endereço(ip) e porta especificamente
 *  Inclusive do servidor primario e o de backup que está recebendo
**/
void send_servers_to_new(){}

/**
 * Envia toda a lista de clientes conhecidos pro novo server backup criado
 * Precisa decidir quais dados enviar exatamente
 * Talvez sicronizar as pastas de cada cliente entre servidor primario e o novo backup aqui
 **/
void send_clients_to_new(){}

/**
 * Envia id, endereço(ip) e porta do novo server backup criado para toda a lista de servers conhecidos
 * Tem que tomar cuidado pra não enviar pra si mesmo e o servidor de backup novo
 **/
void send_new_backup_addr_all(){}


/**
 * Thread de comunicação do servidor primario com um servidor de backup 
 **/
void* serverThread() {
    //send_servers_to_new
    //send_clients_to_new
    //send_new_backup_addr_all -- Talvez colocar essa chamada em outro lugar

	//if(NEW_CLIENT) send_new_client_to_backup
	//if(NEW_BACKUP) send_new_backup_to_backup


    return NULL;
}


//====================================FUNÇÕES SERVIDOR BACKUP=================================================================
void run_backup(char* address, int port, char* primary_server_address, int primary_server_port) {
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

	//Salvar próprio server id em algum lugar
	
	//wait_contact
}

/**
 * Recebe lista de servidores conhecidos do servidor primário
 * Recebe id, endereço(ip) e porta, e armazena na sua própria lista
 **/ 
void recv_server_list(){}


/**
 * Recebe lista de clientes conhecidos do servidor primário
 * Recebe dados e armazena na sua própria lista
 **/ 
void recv_client_list(){}


/**
 * Espera comandos do servidor primário ou de outros backups
 **/ 
void wait_contact(){
	//recv_server_list
	//recv_client_list
	
	//while(TRUE)
		//if(NEW_CLIENT_SIGNAL) recv_new_client
		//if(NEW_BACKUP_SIGNAL) recv_new_backup
		//if(VOTE_SIGAL) start_election
		
		//if(UPLOAD) receive_file
		//if(DOWNLOAD) faz nada
		//if(DELETE) remove_file

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



#endif