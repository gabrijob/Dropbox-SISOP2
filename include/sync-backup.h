#ifndef SYNC_BACKUP_HEADER
#define SYNC_BACKUP_HEADER

#include "dropboxServer.h"
#include "sync-server.h"

#define TEST_SERVER_FOLDER "test_syncBox_users"


void send_client_files(int sockid, Client* client_sync, struct sockaddr_in* backup_addr);
void get_client_files(int sockid);
void synchronize_local(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id);
void synchronize_remote(int sockid, struct sockaddr_in* prim_addr, char* client_id, MSG_ID *msg_id);

#endif