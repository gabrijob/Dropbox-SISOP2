#ifndef SERVER_HEADER
#define SERVER_HEADER

#include "dropboxUtil.h"

void sync_server(int sock_s, Client *client_s, MSG_ID* msg_id);

void receive_file(char *filename, int sockid, char* id, MSG_ID* msg_id, pthread_mutex_t *file_mutex);

void send_file(char *filename, int sockid, char* id, struct sockaddr_in *cli_addr, MSG_ID* msg_id, pthread_mutex_t *file_mutex);

#endif
