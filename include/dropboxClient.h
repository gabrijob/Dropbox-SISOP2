#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "dropboxUtil.h"
#include "sync-client.h"
#include "sync-threads.h"


int login_server(char *host, int port, UserInfo *user, MSG_ID *msg_id);

void sync_client(UserInfo *user, MSG_ID *msg_id);

void send_file(char *path, UserInfo *user, MSG_ID *msg_id, int server_only);

void get_file(char *filename, UserInfo *user, char *path_download, MSG_ID *msg_id);

void delete_file(char *filename, UserInfo *user, MSG_ID *msg_id, int server_only);

void close_session(UserInfo *user, MSG_ID *msg_id);


#endif
