#ifndef SYNC_THREADS_CODE
#define SYNC_THREADS_CODE


#include "sync-threads.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + MAXNAME))

void *answer_pending(void* user) {
	UserInfo *user_info = (UserInfo*) user;
	MSG_ID *msg_id_ptr = user_info->msg_id;

	int sockid = user_info->socket_id;
	struct sockaddr_in *serv_conn = user_info->serv_conn;
	struct sockaddr_in from;

	char buffer[BUFFER_SIZE];

	while(1){
		pthread_mutex_lock(&user_info->lock_server_comm);
			
		/* Ask server if there are pending changes */
		strcpy(buffer, NEED_SYNC);
		if(send_packet(&msg_id_ptr->client, buffer, sockid, serv_conn) < 0) 
			printf("\nERROR asking server if there are pending changes");

		bzero(buffer, BUFFER_SIZE);
		if(recv_packet(&msg_id_ptr->server, buffer, sockid, &from) < 0)
			printf("\nERROR receiving sync request from server");

		/* If there are, synchronize */
		if(strcmp(buffer, SYNC_REQ) == 0) {
			printf("\n\nPending changes on server\n"); //debug
			sync_client(user_info, msg_id_ptr);
		}
		/*else if(strcmp(buffer, SYNC_NREQ) == 0)
			printf("\nNo changes on server");*/ //debug

		pthread_mutex_unlock(&user_info->lock_server_comm);
		usleep(1000000);
	}

	return SUCCESS;
}




/* Uses inotify to watch sync time in a certain period of time */
void *watcher(void* user) {
	UserInfo *user_info = (UserInfo*) user;

	char* watch_path = malloc(strlen((char*) user_info->folder));
	strcpy(watch_path, (char*) user_info->folder);

	int fd, wd;

	fd = inotify_init();
	if(fd < 0) {
		printf("Error inotify_init\n");		
	}
	
	wd = inotify_add_watch(fd, watch_path, IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_DELETE_SELF);

	char path[MAXNAME];
	int thread_running = TRUE;
	int changes;

	while(thread_running) {
		int length, i = 0;
		char buffer[EVENT_BUF_LEN];
		changes = 0;

		length = read(fd, buffer, EVENT_BUF_LEN); 
		pthread_mutex_lock(&(user_info->lock_server_comm));

		if (length < 0) {
			thread_running = FALSE;
	    } else {
	    	i = 0;
	     	while (i < length) {
				struct inotify_event* event = (struct inotify_event *) &buffer[i];

				if (event->len) {
					sprintf(path, "%s/%s", user_info->folder, event->name);

					if (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
						/*if(IN_CLOSE_WRITE) printf("\nIN_CLOSE_WRITE %s", path);
						if(IN_MOVED_TO) printf("\nIN_MOVED_TO %s", path);
						if(IN_DELETE) printf("\nIN_DELETE %s", path);
						if(IN_DELETE_SELF) printf("\nIN_DELETE_SELF %s", path);
						if(IN_MOVED_FROM) printf("\nIN_MOVED_FROM %s", path);*/
						if(event->name[0] != '.')
							changes++;
					}
						/*if(IN_CLOSE_WRITE) printf("\nIN_CLOSE_WRITE %s", path);
						if(IN_MOVED_TO) printf("\nIN_MOVED_TO %s", path);
						if (check_dir(path) && (event->name[0] != '.')) {
							printf("\nRequest upload: %s to user_%s\n", event->name, user_info->id);

							send_file(path, user_info, user_info->msg_id, TRUE);
						}
					} 
					if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
						if(IN_DELETE) printf("\nIN_DELETE %s", path);
						if(IN_DELETE_SELF) printf("\nIN_DELETE_SELF %s", path);
						if(IN_MOVED_FROM) printf("\nIN_MOVED_FROM %s", path);
						if (!check_dir(path)  && event->name[0] != '.') {
							printf("\nRequest delete: %s to user_%s\n", event->name, user_info->id);

							delete_file(event->name, user_info, user_info->msg_id, TRUE);;
						}
					}*/					
				}

				i += EVENT_SIZE + event->len;
      		}
			if(changes > 0) {
				printf("\nRequesting server update");
				update_server(user_info, user_info->msg_id);
			}

		}
		pthread_mutex_unlock(&(user_info->lock_server_comm));;
		usleep(10000000);
	}

	inotify_rm_watch(fd, wd);
	close(fd);

	return SUCCESS;
}

#endif