#ifndef SERVER_REPLICATION_HEADER
#define SERVER_REPLICATION_HEADER

#include "dropboxServer.h"
#include "sync-backup.h"

#define MAXSERVERS 100

#define BACKUP1_ADDRESS "127.0.0.2"
#define BACKUP2_ADDRESS "127.0.0.3"

#define RECV_TIMEOUT 3

#define SL_REQ "SERVERS LIST REQUEST"
#define CL_REQ "CLIENTS LIST REQUEST"
#define NS_SIGNAL "NEW BACKUP SERVER SIGNAL"
#define NC_SIGNAL "NEW CLIENT SIGNAL"
#define UP_SIGNAL "UPLOAD SIGNAL"
#define DEL_SIGNAL "DELETE SIGNAL"
#define GSD_SIGNAL "GET SYNC DIR SIGNAL"
#define TST_CON "TESTING CONNECTION"
#define DM "ELECTION"


struct server_connection{
	int sid;
    int port;
	char address[MAXNAME];
    int socket;
};
typedef struct server_connection s_Connection;

struct client_data{
    char id[MAXNAME];
    struct sockaddr_in *sock_addr;
};
typedef struct client_data C_DATA;

struct propagate_command_data{
	int flag;
	char client_id[MAXNAME];
	char filename[MAXNAME];
};
typedef struct propagate_command_data prog_comm_info;



void init_server_structs(int sid, int port, char* address);
void register_client_login(char *id, struct sockaddr_in *cli_addr, Client* client);
void new_backup_sv_conn(char* my_address, int old_sockid);
int send_servers_to_new(int sockid);
int send_clients_to_new(int sockid);
int send_new_server_to_backup(int to_sid);
void propagate_upload(char *filename, char *client_id);
void propagate_delete(char *filename, char *client_id);
void propagate_sync_dir(char *client_id);
void* serverThread(void* connection_struct);

void run_backup(int sockid, char* address, int port, char* primary_server_address, int primary_server_port);
int recv_servers_list(int sockid, struct sockaddr_in *prim_sv);
int recv_clients_list(int sockid, struct sockaddr_in *prim_sv);
int recv_new_server(int sockid);
void wait_contact(int sockid);
void send_test_msg();
int start_election(int sockid);
void wait_connection_v2();

void serversListPrint();
void clientsListPrint();

#endif
