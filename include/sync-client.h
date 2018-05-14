#ifndef SYNC_CLIENT_HEADER
#define SYNC_CLIENT_HEADER

#include "dropboxUtil.h"
#include "dropboxClient.h"

void synchronize_local(UserInfo user);
void synchronize_remote(int sockid, struct sockaddr_in serv_addr, UserInfo user);

#endif