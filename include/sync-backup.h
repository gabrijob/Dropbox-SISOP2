#ifndef SYNC_BACKUP_HEADER
#define SYNC_BACKUP_HEADER

#include "dropboxServer.h"

#define TEST_SERVER_FOLDER "test_syncBox_users"

#define TSO "TOTAL SYNCHRONIZATION ON"
#define TSF "TOTAL SYNCHRONIZATION FINISHED"


void send_all_clients(int sockid, struct sockaddr_in* backup_addr);
void get_all_clients(int sockid);
void send_client_files(int sockid, char* client_id, struct sockaddr_in* backup_addr);
void get_client_files(int sockid);
void get_file(char *filename, int sockid, struct sockaddr_in* prim_addr, char *client_id, MSG_ID *msg_id);
int delete_file(char* filename, char* client_id);

#endif