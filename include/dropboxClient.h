#ifndef CLIENT_HEADER
#define CLIENT_HEADER

#include "dropboxUtil.h"

/*typedef struct user_info {
	char id[MAXNAME];
  	char folder[MAXNAME*2];
}UserInfo;*/


int login_server(char *host, int port);

void sync_client();

void send_file(char *file);

void get_file(char *file);

void delete_file(char *file);

void close_session();


#endif
