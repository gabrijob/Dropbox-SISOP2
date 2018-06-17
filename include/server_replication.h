#ifndef SERVER_REPLICATION_HEADER
#define SERVER_REPLICATION_HEADER

#include "dropboxServer.h"

#define MAXSERVERS 100

#define BACKUP1_ADDRESS "127.0.0.2"
#define BACKUP2_ADDRESS "127.0.0.3"

struct server_connection{
	int sid;
    int port;
	char address[MAXNAME];
    int socket;
};
typedef struct server_connection s_Connection;


void new_backup_sv_conn(char* my_address, int old_sockid, struct sockaddr_in *sv_addr);
void run_backup(char* address, int port, char* primary_server_address, int primary_server_port);



#endif