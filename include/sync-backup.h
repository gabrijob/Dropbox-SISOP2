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


#endif