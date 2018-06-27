#ifndef SYNC_SERVER_HEADER
#define SYNC_SERVER_HEADER

#include "dropboxServer.h"

void synchronize_client(int sockid, Client* client_sync, MSG_ID* msg_id);
void synchronize_server(int sockid, Client* client_sync, MSG_ID* msg_id);
void update_del(int sockid, Client* client_sync, MSG_ID* msg_id);
void update_get(int sockid, Client* client_sync, MSG_ID* msg_id);

#endif