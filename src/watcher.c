#ifndef WATCHER_CODE
#define WATCHER_CODE


#include "watcher.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE))

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

		      					send_file_client(path, user_info);
								pthread_mutex_unlock(&(user_info->lock_server_comm));
		    				}
		  			} else if (event->mask & (IN_DELETE | IN_DELETE_SELF | IN_MOVED_FROM)) {
	    					if (event->name[0] != '.') {
	      						pthread_mutex_lock(&(user_info->lock_server_comm));
								printf("\nRequest delete: %s to user_%s\n", event->name, user_info->id);

	      						delete_file(event->name, user_info);
								pthread_mutex_unlock(&(user_info->lock_server_comm));
	    					}
		  			}
				}

				i += EVENT_SIZE + event->len;
	      		}
		}

		usleep(100);
	}

	inotify_rm_watch(fd, wd);
	close(fd);

	return SUCCESS;
}

#endif