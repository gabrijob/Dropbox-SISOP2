#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "dropboxUtil.h"
#include "sync-client.h"
#include "watcher.h"


int login_server(char *host, int port);

void sync_client();

void send_file_client(char *filename, UserInfo user);

void get_file(char *filename, UserInfo user);

void delete_file(char *file);

void close_session();


#endif
