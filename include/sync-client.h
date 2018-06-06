#ifndef SYNC_CLIENT_HEADER
#define SYNC_CLIENT_HEADER

#include "dropboxUtil.h"
#include "dropboxClient.h"

void synchronize_local(UserInfo *user, MSG_ID *msg_id);
void synchronize_remote(UserInfo *user, MSG_ID *msg_id);

#endif