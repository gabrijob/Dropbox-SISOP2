#ifndef CLIENT_CODE
#define CLIENT_CODE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


int login_server(int userID, char* host, int port){
	int sockfd;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;


	server = gethostbyname(host);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }	

	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(port);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]);
	bzero(&(serv_addr.sin_zero), 8);

	//GET SOCKET
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");


	unsigned int length = sizeof(struct sockaddr_in);
	int n;

	//TEST CONNECTION WITH SERVER
	char message[20] = "CONNECT";
	n = sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *) &serv_addr, length);
	if (n < 0) 
		printf("ERROR ESTABLISHING CONNECTION WITH SERVER");

	//RECEIVE ACK FROM SERVER
    n = recvfrom(sockfd, message, 20, 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		printf("ERROR GETTING RESPONSE FROM SERVER");

	printf("Got an ack: %s\n", message);

	//SEND USERID
	sprintf(message, "%i", userID);
	n = sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *) &serv_addr, length);
	if (n < 0)
		printf("ERROR SENDING USERID");
	
	
	//CLOSE
	close(sockfd);
	return 0;
}

int main(int argc, char *argv[]){

	if (argc < 4) {
		fprintf(stderr, "usage %s userID hostname port\n", argv[0]);
		exit(0);

	}

	return login_server(strtol(argv[1], NULL, 10), argv[2], strtol(argv[3], NULL, 10));

}


#endif