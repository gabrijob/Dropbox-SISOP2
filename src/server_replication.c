#ifndef SERVER_REPLICATION_CODE
#define SERVER_REPLICATION_CODE

#include "server_replication.h"


//----------------------GLOBAL VARIABLES-----------------------------------------------------------------------
int my_id = 0;

int last_new_id = 0;
int new_primary = 0;
s_Connection* serversList[MAXSERVERS]; // id do servidor é sua posição no array
int last_client_index = -1;
C_DATA* clientsList[MAX_CLIENTS];


/*-----------Signals-------------------*/
int is_new_backup[MAXSERVERS];
int is_new_client[MAXSERVERS];
prog_comm_info is_upload[MAXSERVERS];
prog_comm_info is_delete[MAXSERVERS];
prog_comm_info is_sync_dir[MAXSERVERS];
/*-------------------------------------*/

sem_t s_propagation;

int primary_killed = FALSE;
static sigjmp_buf recv_timed_out;
static sigjmp_buf recv_timed_out2;
static sigjmp_buf recv_timed_out3;
//-------------------------------------------------------------------------------------------------------------

/* Timeout handler functions */
void timeout_handler(int sig) {

	signal(SIGALRM, SIG_DFL);	
	if(primary_killed == FALSE) {
		printf("\n-------Primary killed--------\n");
		siglongjmp(recv_timed_out, 1);
	}
	
}
void timeout_backup_handler(int sig) {

	signal(SIGALRM, SIG_DFL);	
	siglongjmp(recv_timed_out2, 1);
}
void timeout_backup_handler2(int sig) {

	signal(SIGALRM, SIG_DFL);	
	siglongjmp(recv_timed_out3, 1);
}

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

//====================================FUNÇÕES SERVIDOR PRIMARIO=================================================================
void init_server_structs(int sid, int port, char* address) {

	/* Insert primary server on servers list*/
	s_Connection *connection = (s_Connection*) malloc(sizeof(s_Connection));
	connection->sid = sid;
	connection->port = port;
	strcpy(connection->address, address);

	serversList[sid] = connection;

	/* Initialize list of flags for signal lists*/
	for(int i = 0; i < MAXSERVERS; i++) {
		is_new_backup[i] = FALSE;
		is_new_client[i] = FALSE;
		is_upload[i].flag = FALSE;
		is_delete[i].flag = FALSE;
		is_sync_dir[i].flag = FALSE;
	}

}

/* Indicates to whom primary server must send the new backup server data */
void signal_new_backup() {
	for(int i = 0; i < last_new_id; i++) {
		if((serversList[i] != NULL) && (serversList[i]->sid != -1) && (i != my_id))
			is_new_backup[i] = TRUE;
	}
}

/* Indicates to whom primary server must send the client data */
void signal_new_client() {
	for(int i = 0; i <= last_new_id; i++) {
		if((serversList[i] != NULL) && (serversList[i]->sid != -1) && (i != my_id)) {
			is_new_client[i] = TRUE;
		}
	}
}

/* Indicates to whom primary server must send upload request */
void signal_upload(char *client_id, char *filename) {
	int counter = 0;
	for(int i = 0; i <= last_new_id; i++) {
		if((serversList[i] != NULL) && (serversList[i]->sid != -1) && (i != my_id)) {
			is_upload[i].flag = TRUE;
			strcpy(is_upload[i].client_id, client_id);
			strcpy(is_upload[i].filename, filename);
			counter++;
		}
	}
	sem_init(&s_propagation, 0, -counter);
}

/* Indicates to whom primary server must send delete request */
void signal_delete(char *client_id, char *filename) {
	int counter = 0;
	for(int i = 0; i <= last_new_id; i++) {
		if((serversList[i] != NULL) && (serversList[i]->sid != -1) && (i != my_id)) {
			is_delete[i].flag = TRUE;
			strcpy(is_delete[i].client_id, client_id);
			strcpy(is_delete[i].filename, filename);
			counter++;
		}
	}
	sem_init(&s_propagation, 0, -counter);
}

/* Indicates to whom primary server must synchronize client directory */
void signal_sync_dir(char *client_id) {
	int counter = 0;
	for(int i = 0; i <= last_new_id; i++) {
		if((serversList[i] != NULL) && (serversList[i]->sid != -1) && (i != my_id)) {
			is_sync_dir[i].flag = TRUE;
			strcpy(is_sync_dir[i].client_id, client_id);
			counter++;
		}
	}
	sem_init(&s_propagation, 0, -counter);
}


/* Insert client data on the clients list */
void register_client_login(char *id, struct sockaddr_in *cli_addr, Client* client) {
	C_DATA *client_data = (C_DATA*) malloc(sizeof(C_DATA));
	strcpy(client_data->id, id);
	client_data->sock_addr = cli_addr;

	last_client_index++;
	clientsList[last_client_index] = client_data;
	signal_new_client();
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

		/* Send id */
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

void send_test_msg () {

	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%s", TST_CON);
	
	for (int i = 1; i<=last_new_id; i++)
		if(serversList[i] != NULL) {
			//printf("\nserver list %d not null", i);		//debug
	
			struct sockaddr_in backup_addr;
			s_Connection *to_backup = serversList[i];

			int to_socket = to_backup->socket;
			int to_port = to_backup->port;
			char to_address[MAXNAME];
			strcpy(to_address, to_backup->address);
	
			/* Create struct for server socket address */
			if(init_server_connection(to_port, to_address, NULL, &backup_addr) == ERROR) {
				printf("\nERROR initiating server connection...exiting");
				exit(1);
			}
			/* Send new backup server signal to backup server */
			int zero = START_MSG_COUNTER;

			/* If backup does not respond, handler */
			if (sigsetjmp(recv_timed_out3, 1)) {
				serversList[i] = NULL;
				printf("\nBackup killed");				
			}
	
			//if can't send anything in 3 secs the backup was probably killed
			signal(SIGALRM, timeout_backup_handler2);
			alarm(RECV_TIMEOUT); 
			if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
				printf("\nERROR sending sid to backup server");
			alarm(0);
			signal(SIGALRM, SIG_DFL);
		}
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
		printf("\nERROR sending signal to backup server");	

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

int send_new_client_to_backup(int to_sid) {
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
	
	C_DATA *new_client = clientsList[last_client_index];

	if(new_client == NULL) {
		printf("\nERROR new client unavailable");
		return ERROR;
	}

	/* Send new client signal to backup server */
	sprintf(buffer, "%s", NC_SIGNAL);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending client id to backup server");	

	char client_id[MAXNAME];
	char* sock_addr;
	
	strcpy(client_id, new_client->id);

	/* Send id */
	strcpy(buffer, client_id);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending client id to backup server");


	sock_addr = (char*) new_client->sock_addr;
	/* Send socket address */
	memcpy(buffer, sock_addr, BUFFER_SIZE);
	zero = START_MSG_COUNTER;
	if(send_packet(&zero, buffer, to_socket, &backup_addr) < 0)
		printf("\nERROR sending socket address to backup server");
	
	is_new_client[to_sid] = FALSE;

	/* Synchronize client files with backup*/
	send_client_files(to_socket, client_id, &backup_addr);
	//sem_post(&s_focus_client);

	return SUCCESS;	
}

void propagate_upload(char *filename, char *client_id) {
	printf("\nPropagating upload to backups");		//debug
	signal_upload(client_id, filename);
	sem_wait(&s_propagation);
	sem_destroy(&s_propagation);
}

void propagate_delete(char *filename, char *client_id) {
	printf("\nPropagating delete to backups");		//debug
	signal_delete(client_id, filename);
	sem_wait(&s_propagation);
	sem_destroy(&s_propagation);
}

void propagate_sync_dir(char *client_id) {
	printf("\nPropagating sync dir to backups");		//debug
	signal_sync_dir(client_id);
	sem_wait(&s_propagation);
	sem_destroy(&s_propagation);
}

/**
 * Thread de comunicação do servidor primario com um servidor de backup 
 **/
void* serverThread(void* connection_struct) {
	
	s_Connection *connection = (s_Connection*) connection_struct;
	
	int sockid = connection->socket;
	int sid = connection->sid;
	int port = connection->port;
	char address[MAXNAME];
	sprintf(address, "%s", connection->address);

	struct sockaddr_in backup_addr;

    send_servers_to_new(sockid);
	serversListPrint(); 			//debug
	send_clients_to_new(sockid);
	clientsListPrint();				//debug

	/* Create struct for server socket address */
	if(init_server_connection(port, address, NULL, &backup_addr) == ERROR) {
		printf("\nERROR initiating server connection");
	}

	send_all_clients(sockid, &backup_addr);

	pthread_mutex_t file_mutex;
	pthread_mutex_init(&file_mutex, NULL);
	MSG_ID msg_id;
	int zero;
	char buffer[BUFFER_SIZE];

	while(TRUE)  {
		
		if(is_new_backup[sid]) {
			send_new_server_to_backup(sid);
		}
		else if(is_new_client[sid]) {
			send_new_client_to_backup(sid);
		}
		else if(is_upload[sid].flag) {
			/* Send upload signal to backup server */
			sprintf(buffer, "%s", UP_SIGNAL);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending signal to backup server");	

			/* Send client id */
			strcpy(buffer, is_upload[sid].client_id);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending client id to backup server");

			/* Send filename */
			strcpy(buffer, is_upload[sid].filename);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending filename to backup server");

			/* Send file*/
			msg_id.client = 0;
    		msg_id.server = 0;
			if(recv_packet(&msg_id.client, buffer, sockid, &backup_addr) < 0)
        		printf("\nERROR receiving request");
			strcpy(buffer, OK);
        	if(send_packet(&msg_id.server, buffer, sockid, &backup_addr) < 0)
        		printf("\nERROR telling to not send filename");

			send_file(is_upload[sid].filename, sockid, is_upload[sid].client_id, &backup_addr, &msg_id, &file_mutex);
			is_upload[sid].flag = FALSE;
			sem_post(&s_propagation);
		}	
		else if(is_delete[sid].flag) {
			/* Send delete signal to backup server */
			sprintf(buffer, "%s", DEL_SIGNAL);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending signal to backup server");

			/* Send client id */
			strcpy(buffer, is_delete[sid].client_id);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending client id to backup server");

			/* Send filename */
			strcpy(buffer, is_delete[sid].filename);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending filename to backup server");
			
			is_delete[sid].flag = FALSE;
			sem_post(&s_propagation);				
		}
		else if(is_sync_dir[sid].flag) {
			/* Send get sync dir signal to backup server */
			sprintf(buffer, "%s", GSD_SIGNAL);
			zero = START_MSG_COUNTER;
			if(send_packet(&zero, buffer, sockid, &backup_addr) < 0)
				printf("\nERROR sending signal to backup server");

			send_client_files(sockid, is_sync_dir[sid].client_id, &backup_addr);
			is_sync_dir[sid].flag = FALSE;
			sem_post(&s_propagation);	
		}	
		else {
			sleep(2);
			send_test_msg();
		}		
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
	serversList[my_id]->socket = sockid;

	get_all_clients(sockid);

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
 * Receive new client data and stores it on servers list 
 **/ 
int recv_new_client(int sockid) {
	int zero;
	char buffer[BUFFER_SIZE];
	char client_folder[MAXPATH];

	struct sockaddr_in prim_sv;
	
	C_DATA *client_data = (C_DATA*) malloc(sizeof(C_DATA));

	/* Receives client id  */
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_sv) < 0)
		printf("\nERROR receiving client id"); 
		
	strcpy(client_data->id, buffer);

	/* Receive client socket address*/
	zero = START_MSG_COUNTER;
	bzero(buffer, BUFFER_SIZE -1);
	if(recv_packet(&zero, buffer, sockid, &prim_sv) < 0)
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

	last_client_index++;
	clientsList[last_client_index] = client_data;

	/* Synchornize client directory with main server's */
	get_client_files(sockid);

	return SUCCESS;
}

/**
 * Espera comandos do servidor primário ou de outros backups
 **/ 
void wait_contact(int sockid) {
	MSG_ID msg_id;
    msg_id.client = 0;
    msg_id.server = 0;
	int zero;
	char buffer[BUFFER_SIZE];

	struct sockaddr_in conn_addr;


	while(TRUE) {

		/* Receive an order */
		bzero(buffer, BUFFER_SIZE -1);
		zero = START_MSG_COUNTER;

		//printf("\nloop");

		/* Save current state of process in the stack */
		if (sigsetjmp(recv_timed_out, 1)) {
			/* starts election */
			primary_killed = TRUE;
			alarm(0);
			signal(SIGALRM, SIG_DFL);
			int return_buffer = start_election(sockid);
			printf("\nElection returned %d", return_buffer);
			if (return_buffer == ERROR) {
				printf("\nNew primary could not be selected... exiting");
				exit(1);
			}
			else if (return_buffer == SUCCESS)
				break;

			/* Return buffer contains new primary id */
			else {
				new_primary = return_buffer;
				
				//mandar dados do novo primario -> id dele esta em new_primary
				
				break;
			}	
			
		}
		
		//if nothing is received in 3 secs the server was probably killed
		signal(SIGALRM, timeout_handler);
		alarm(RECV_TIMEOUT);

		if(recv_packet(&zero, buffer, sockid, &conn_addr) == 0) {
			//printf("\nReceived datagram %s\n", buffer);
			alarm(0);
			signal(SIGALRM, SIG_DFL);
		}

		/* NEW BACKUP SERVER */
		if(strcmp(buffer, NS_SIGNAL) == 0) { 
			recv_new_server(sockid);
			serversListPrint(); 			//debug
		}

		/* NEW CLIENT */
		if(strcmp(buffer, NC_SIGNAL) == 0) {
			recv_new_client(sockid);
			clientsListPrint();				//debug
		}
		
		/* UPLOAD */
		if(strcmp(buffer, UP_SIGNAL) == 0) {
			/* Receive client_id*/
			bzero(buffer, BUFFER_SIZE -1);
			zero = START_MSG_COUNTER;
			if(recv_packet(&zero, buffer, sockid, &conn_addr) < 0)
				printf("\nERROR receiving client id");
			
			char client_id[MAXNAME];
			sprintf(client_id, "%s", buffer);

			/* Receive filename*/
			bzero(buffer, BUFFER_SIZE -1);
			zero = START_MSG_COUNTER;
			if(recv_packet(&zero, buffer, sockid, &conn_addr) < 0)
				printf("\nERROR receiving filename");
			
			char filename[MAXNAME];
			sprintf(filename, "%s", buffer);

			/* Download file */
			msg_id.client = 0;
    		msg_id.server = 0;
			get_file(filename, sockid, &conn_addr, client_id, &msg_id);
		}

		/* DELETE */
		if(strcmp(buffer, DEL_SIGNAL) == 0) {
			/* Receive client_id*/
			bzero(buffer, BUFFER_SIZE -1);
			zero = START_MSG_COUNTER;
			if(recv_packet(&zero, buffer, sockid, &conn_addr) < 0)
				printf("\nERROR receiving client id");
			
			char client_id[MAXNAME];
			sprintf(client_id, "%s", buffer);

			/* Receive filename*/
			bzero(buffer, BUFFER_SIZE -1);
			zero = START_MSG_COUNTER;
			if(recv_packet(&zero, buffer, sockid, &conn_addr) < 0)
				printf("\nERROR receiving filename");
			
			char filename[MAXNAME];
			sprintf(filename, "%s", buffer);

			delete_file(filename, client_id);
		}

		/* GET_SYNC_DIR */
		if(strcmp(buffer, GSD_SIGNAL) == 0) {
			get_client_files(sockid);
		}

		//if(strcmp(buffer, TST_CON) == 0) { 
			//recv_new_server(sockid);
			//serversListPrint(); 
						
		//}		
	}
	/* End of first infinite loop */
	wait_connection_v2();
}

/* Tests connection between backups */
void wait_connection_v2() {
	
	int j = 1; //default

	if (new_primary != 0) {
		printf("\nI am the new primary\n");  

		int sockid = serversList[my_id]->socket;

		while(TRUE) {
			
			char buffer[BUFFER_SIZE];
			struct sockaddr_in sv_conn_backup;

			int zero = START_MSG_COUNTER;
			bzero(buffer, BUFFER_SIZE -1);
			
			sleep(2);
			if(recv_packet(&zero, buffer, sockid, &sv_conn_backup) == 0) {
				printf("\nReceived backup request for %s\n", buffer);
			}
				
			else {
				printf("\nERROR receiving democracy request");
			}

		} 


	}

	if (new_primary == 0) {
		printf("\nI am still a backup :(\n");
		for(int i = 1; i<= last_new_id; i++) {
			if (i != my_id) {
				j = i;
				break; //j is the new speaker
			}
		}
		int sockid;
		struct sockaddr_in sv_conn;
		sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sockid == ERROR) {
			printf("\nError opening socket");
		}
		
		bzero((char *) &sv_conn, sizeof(struct sockaddr_in));
		sv_conn.sin_family = AF_INET;
		sv_conn.sin_port = htons(serversList[j]->port);
		sv_conn.sin_addr.s_addr = inet_addr(serversList[j]->address);

		
		while(TRUE) {

			char buffer[BUFFER_SIZE];
			int zero = START_MSG_COUNTER;

			bzero(buffer, BUFFER_SIZE -1);
			strcpy(buffer, TST_CON);
			if (sigsetjmp(recv_timed_out2, 1)) {
				printf("\nNo signal received! I am now the only server available\n");
				new_primary = my_id;
				//mandar os dados do novo primário para se conectar ao cliente-> id esta em new_primary

				exit(1); //debug
			}

			printf("\nStarting connection with backup server at %s/%d\n", serversList[j]->address, serversList[j]->port);
			sleep(2);

			signal(SIGALRM, timeout_backup_handler);
			alarm(RECV_TIMEOUT);
			if(send_packet(&zero, buffer, sockid, &sv_conn) < 0) {
				printf("\nERROR starting connection with backup server");
			}
			else {
				alarm(0);
				signal(SIGALRM, SIG_DFL);
			}
		}
	}
}

int start_election(int n_sock) {

	puts("election reached");
	int n_backups = last_new_id;
	int speaker = 0;

	for (int i = 1; i<=n_backups; i++) {
		if(serversList[i] != NULL) {
			speaker = i;		
			break;
		}
	}

	for (int j = (speaker+1); j<=n_backups; j++) {
		if(serversList[j] != NULL) {
			if (my_id == speaker) {
				sleep(2);
				
				int sockid;
				char buffer[BUFFER_SIZE];
				int zero = START_MSG_COUNTER;

				bzero(buffer, BUFFER_SIZE -1);
				strcpy(buffer, DM);

				struct sockaddr_in sv_conn, from;
		
			    	sockid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (sockid == ERROR) {
					printf("\nError opening socket in start_election()");
					return ERROR;
				}
				bzero((char *) &sv_conn, sizeof(struct sockaddr_in));
				bzero((char *) &from, sizeof(struct sockaddr_in));
				sv_conn.sin_family = AF_INET;
				sv_conn.sin_port = htons(serversList[j]->port);
				sv_conn.sin_addr.s_addr = inet_addr(serversList[j]->address);

				printf("\nStarting connection with backup server at %s/%d", serversList[j]->address, serversList[j]->port);
				if(send_packet(&zero, buffer, sockid, &sv_conn) < 0) {
					printf("\nERROR starting connection with backup server");
					return ERROR;
				}

				zero = START_MSG_COUNTER;
				bzero(buffer, BUFFER_SIZE -1);
				/* receives feedback */
				if(recv_packet(&zero, buffer, sockid, &from) == 0) {
					printf("\n%s", buffer);
				}
				else
					return ERROR;

				return SUCCESS;
			}

			else if (my_id == j) {
				
				char buffer[BUFFER_SIZE];
				struct sockaddr_in sv_conn_backup;

				int zero = START_MSG_COUNTER;
				bzero(buffer, BUFFER_SIZE -1);

				if(recv_packet(&zero, buffer, n_sock, &sv_conn_backup) == 0) {
					printf("\nReceived backup request for %s", buffer);
				}
					
				else {
					printf("\nERROR receiving democracy request");
					return ERROR;
				}
				/* Sends feedback */
				zero = START_MSG_COUNTER;
				bzero(buffer, BUFFER_SIZE -1);
				strcpy(buffer, "Feedback ok from backup 2");
				if(send_packet(&zero, buffer, n_sock, &sv_conn_backup) < 0) {
					printf("ERROR sending feedback\n");
					return ERROR;
				}
				else {
					return my_id; //new leader id
				}				
				
			}	
		}	
	}
	
	return SUCCESS;

}

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
		printf("\n\tID: %s\n\n", clientsList[i]->id);

	}
}


#endif
