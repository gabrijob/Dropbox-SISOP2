#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "dropboxUtil.h"


int login_server(char *host, int port);

void sync_client();

void send_file(char *file, int sockid, struct sockaddr_in *serv_conn);

void get_file(char *file);

void delete_file(char *file);

void close_session();


#endif
