#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "dropboxUtil.h"
#include "sync-client.h"
#include "watcher.h"


int login_server(char *host, int port, UserInfo *user);

void sync_client(UserInfo *user);

void send_file_client(char *path, UserInfo *user);

void get_file(char *filename, UserInfo *user);

void delete_file(char *filename, UserInfo *user);

void close_session(UserInfo *user);


#endif
