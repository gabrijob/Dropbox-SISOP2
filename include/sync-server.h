#ifndef SYNC_SERVER_HEADER
#define SYNC_SERVER_HEADER

#include "dropboxServer.h"

void synchronize_client(int sockid, Client* client_sync);
void synchronize_server(int sockid_sync, Client* client_sync);

#endif