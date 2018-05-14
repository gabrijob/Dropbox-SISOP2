#ifndef UTIL_CODE
#define UTIL_CODE

#include "dropboxUtil.h"
#include "sync-server.h"
#include "sync-client.h"
#include "watcher.h"


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LICENSE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
void *dir_content_thread(void *ptr) {
   struct dir_content *args = (struct dir_content *) ptr;

   get_dir_content(args->path, args->files, args->counter);

   pthread_exit(NULL);
   return NULL;
}

int get_dir_file_info(char * path, FileInfo files[]) {
	struct d_file dfiles[MAXFILES];
  	char path_file[MAXNAME*2 + 1];
  	int counter = 0;

  	get_dir_content(path, dfiles, &counter);

  	for(int i = 0; i < counter; i++) {
  		strcpy(files[i].name, dfiles[i].name);
    		sprintf(path_file, "%s/%s", dfiles[i].path, dfiles[i].name);
  		getModifiedTime((char*) &path_file, (char*) &files[i].last_modified);
    		getFileExtension(dfiles[i].name, (char*) &files[i].extension);
  		files[i].size = getFileSize(dfiles[i].path);
  	}
  	return counter;
}

void getFileExtension(const char *filename, char* extension) {
	const char *dot = strrchr(filename, '.');
  	if(!dot || !strcmp(dot, filename)) {
    		strcpy(extension, "");
  	} else {
  		strcpy(extension, dot+1);
  	}
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int get_dir_content(char *path, struct d_file files[], int* counter) {
	DIR * d = opendir(path);
  	if(d == NULL) {
    		return ERROR;
  	}

  	struct dirent * entry;
  	while (((entry = readdir(d)) != NULL) && (*counter) < MAXFILES) {
    		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
      			struct d_file newFile;
      			strcpy(newFile.path, path);
      			strcpy(newFile.name, entry->d_name);

      			pthread_mutex_lock(&lock);
      			memcpy(&files[(*counter)], &newFile, sizeof(newFile));
      			(*counter)++;
      			pthread_mutex_unlock(&lock);

      			int rc;
      			pthread_t thread;
      			if(entry->d_type == DT_DIR) { // Arquivo é um diretório
        			struct dir_content args;
        			args.path = malloc(sizeof(char) * MAXNAME * 2 + 1); // MAXNAME + / + MAXNAME

        			sprintf(args.path, "%s/%s", newFile.path, newFile.name);
        			args.files = files;
        			args.counter = counter;

        			if((rc = pthread_create(&thread, NULL, &dir_content_thread, (void *) &args))) {
          				printf("Thread creation failed: %d\n", rc);
        			}

        			pthread_join(thread, NULL);
      			}
    		}
  	}

  	closedir(d);
	return 0;
}
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! LICENSE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */



/* Gets modification time of file using lib time.h */
void getModifiedTime(char *path, char *last_modified) {
	struct stat attr;
	stat(path, &attr);

	strftime(last_modified, 20, "%Y.%m.%d %H:%M:%S", localtime(&(attr.st_mtime)));
}

/* Gets current time using lib time.h */
time_t getTime(char *last_modified){
	time_t result = 0;

	int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;

	if (sscanf(last_modified, "%4d.%2d.%2d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec) == 6) {
		struct tm breakdown = { 0 };
		breakdown.tm_year = year - 1900; /* years since 1900 */
		breakdown.tm_mon = month - 1;
		breakdown.tm_mday = day;
		breakdown.tm_hour = hour;
		breakdown.tm_min = min;
		breakdown.tm_sec = sec;

		if ((result = mktime(&breakdown)) == (time_t)-1) {
			fprintf(stderr, "Could not convert time input to time_t\n");
			return EXIT_FAILURE;
		}

		return result;
	} else {
		printf("The input was not a valid time format: %s\n", last_modified);
		return ERROR;
	}
}

/* Gets the oldest fle
	-> returns SUCCESS if aux file is older than the modified date */
int older_file(char *last_modified, char *aux) {
	time_t time_f1 = getTime(last_modified);
	time_t time_f2 = getTime(aux);

	if(difftime(time_f1, time_f2) > 0)
		return SUCCESS;
	return 0;
}

/*
	getpwuid() --> Gets users initial working directory
	geteuid()  --> Gets the effective user ID of the current process
*/

char* getUserHome() {
 	struct passwd *pw = getpwuid(geteuid());	 

	if (pw) {
    		return pw->pw_dir;
  	}
	
	printf("No root directory found! Returning empty...\n");
 	return "";
}

int getFilesize(FILE* file) {
	int size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	return size;
}

int getFileSize(char *path) {
	struct stat attr;
	stat(path, &attr);
	
	return attr.st_size;
}

/* Verfies if the server directory already exists 
   	->Uses the 'sys/stat.h' lib */

bool check_dir(char *pathname) {
	struct stat st = {0};
	
	if (stat(pathname, &st) == ERROR)
		return FALSE;
	
	return TRUE;
}


/* Seraches for client with the userId passed as an arument in the GLOBAL clients list
	-> returns the client's node if found or NULL if not */

Client* searchClient(char* userId, ClientList user_list) {

	ClientList current = user_list;

	while(current != NULL) {
		if(strcmp(userId, current->client->userid) == 0 && current->client->logged_in == 1) {
			return current->client;
		}
		current = current->next;
	}
	return NULL;
}



/* Adds a new client with the userId passed as an arument in the GLOBAL clients list
	-> returns the updated clients list */

ClientList addClient(char* userID, int socket, ClientList user_list) {

	Client* new_client = (Client*) malloc(sizeof(Client));

	strcpy(new_client->userid, userID);
	new_client->devices[0] = socket;
	new_client->devices[1] = -1;
	new_client->logged_in = 1;
	new_client->n_files = 0;

	ClientNode* new_node = (ClientNode*) malloc(sizeof(ClientNode));
	new_node->client = new_client;

	if(user_list == NULL) {
		user_list = new_node;
		new_node->next = NULL;
		new_node->prev = NULL;
	} else {
	ClientNode* current_node = user_list;

	while(current_node->next != NULL) {
	    current_node = current_node->next;
	}

	current_node->next = new_node;
	new_node->prev = current_node;

	}

	return user_list;
}

/**Tries to add a new device to a client, 
 * returns 1 if successfull
 * returns -1 if client reached the max amount of devices
 */
int newDevice(Client* client, int socket) {
	if(client->devices[0] == -1) {
		client->devices[0] = socket;
		return 0;
	}
	if(client->devices[1] == -1) {
		client->devices[1] = socket;
		return 0;
	}

	return -1;
}

int fileExists(char* filename) {
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

/* --------------------------> FOR DEBUG <-------------------------- */
void printUserList(ClientList user_list){

    ClientNode* current_node = user_list;
  
    while(current_node != NULL){
        printf("\n User: %s | Devices:", current_node->client->userid);
        if(current_node->client->devices[0] > 0)
            printf(" socket-%i ", current_node->client->devices[0]);
        if(current_node->client->devices[1] > 0)
            printf(" socket-%i ", current_node->client->devices[1]);
        printf("\n");
        current_node = current_node->next;
    }
}

void printClientFiles(Client* client){
	printf("\nFiles in the client folder:");

	for(int i = 0; i < client->n_files; i++){
		printf("\n\n\tName: %s", client->files[i].name);
		printf("\n\tExtension: %s", client->files[i].extension);
		printf("\n\tLast Modified: %s", client->files[i].last_modified);
		printf("\n\tSize: %d", client->files[i].size);
	}
}

/* ----------------------------------------------------------------- */



#endif
