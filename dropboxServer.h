#ifndef SERVER_HEADER
#define SERVER_HEADER

#include <stdio.h>


#define MAXNAME 200     //talvez mudar eventualmente
#define MAXFILES 50     //talvez mudar eventualmente


struct file_info{
    char name[MAXNAME];
    char extension[MAXNAME];
    char last_modified[MAXNAME];
    int size;
};


struct client{
    int devices[2];
    char userid[MAXNAME];
    struct file_info files[MAXFILES];
    int logged_in;
};



void sync_server();

void receive_file(char* file);

void send_file(char* file);

#endif