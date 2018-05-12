#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "dropboxUtil.h"


int login_server(char *host, int port);

void sync_client();

void send_file(char *filename);

void get_file(char *filename);

void delete_file(char *file);

void close_session();


#endif
