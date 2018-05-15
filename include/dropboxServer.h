#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "dropboxUtil.h"

void sync_server(int sock_s, Client *client_s);

void receive_file(char *filename, int sockid, int id);

void send_file_server(char *filename, int sockid, int id, struct sockaddr_in *cli_addr);

#endif
