#ifndef SYNC_THREADS_CODE
#define SYNC_THREADS_CODE


#include "sync-threads.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE))

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
			//printf("\nPending changes on server"); //debug
			sync_client(user_info, msg_id_ptr);
		}
		/*else if(strcmp(buffer, SYNC_NREQ) == 0)
			printf("\nNo changes on server");*/ //debug

		pthread_mutex_unlock(&user_info->lock_server_comm);
	}

	return SUCCESS;
}




/* Uses inotify to watch sync time in a certain period of time */
void *watcher(void* user) {
	UserInfo *user_info = (UserInfo*) user;

	char* watch_path = malloc(strlen((char*) user_info->folder));
	strcpy(watch_path, (char*) user_info->folder);

	int fd, wd;
	int length, i = 0;
	char buffer[EVENT_BUF_LEN];

	fd = inotify_init();
	if(fd < 0) {
		printf("Error inotify_init\n");		
	}
	
	wd = inotify_add_watch(fd, watch_path, IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM | IN_MOVED_TO);

	char path[MAXNAME];
	int thread_running = TRUE;

	while(thread_running) {
		length = read(fd, buffer, EVENT_BUF_LEN); 

		if (length < 0) {
			thread_running = FALSE;
	    	} else {
	      		i = 0;
	     	 	while (i < length) {
				struct inotify_event* event = (struct inotify_event *) &buffer[i];

				if (event->len) {
		  			sprintf(path, "%s/%s", watch_path, event->name);

		  			if (event->mask & (IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO)) {
		    				if (check_dir(path) && (event->name[0] != '.')) {
		      					pthread_mutex_lock(&(user_info->lock_server_comm));
								printf("\nRequest upload: %s to user_%s\n", event->name, user_info->id);

		      					//send_file_client(path, user_info);
								pthread_mutex_unlock(&(user_info->lock_server_comm));
		    				}
		  			} else if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
	    					if (event->name[0] != '.') {
	      						pthread_mutex_lock(&(user_info->lock_server_comm));
								printf("\nRequest delete: %s to user_%s\n", event->name, user_info->id);

	      						//delete_file(event->name, user_info);
								pthread_mutex_unlock(&(user_info->lock_server_comm));
	    					}
		  			}
				}

				i += EVENT_SIZE + event->len;
	      		}
		}

		usleep(5000000);
	}

	inotify_rm_watch(fd, wd);
	close(fd);

	return SUCCESS;
}

#endif