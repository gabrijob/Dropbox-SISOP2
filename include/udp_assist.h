#ifndef UDP_ASSIST_HEADER
#define UDP_ASSIST_HEADER


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>


#define BUFFER_SIZE 1024
#define MAXIDSIZE 25
#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE !(TRUE)

/* Ack Structure */
/*
	->message_id	:	contains the id of the message, must be incremented every new message
	->ack		:	if the ack is confirmed must be TRUE
	->buffer	:	contains the string content of the message
	->user		:	contains the info of who sent the message (always as client, server user is default)
*/
typedef struct frame{
    int message_id;			
    bool ack;				
    char buffer[BUFFER_SIZE];
    char user[MAXIDSIZE];
}Frame;
/* End of Ack */


int send_packet(int msgid, char *buffer, int sockid, struct sockaddr_in *to);

int recv_packet(int msgid, char *buffer, int sockid, struct sockaddr_in *from);



#endif