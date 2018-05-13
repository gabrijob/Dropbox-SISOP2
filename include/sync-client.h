#ifndef SYNC_CLIENT_HEADER
#define SYNC_CLIENT_HEADER


#include "dropboxClient.h"


void synchronize_local(int sockid, struct sockaddr_in serv_addr, UserInfo user);
void synchronize_remote(int sockid, struct sockaddr_in serv_addr, UserInfo user);

#endif