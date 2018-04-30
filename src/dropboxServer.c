#ifndef SERVER_CODE
#define SERVER_CODE


#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

#include "clientList.h"

int main(int argc, char *argv[]){
	int sockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buf[256];


	if (argc < 2) {
		fprintf(stderr, "usage %s port\n", argv[0]);
		exit(0);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(strtol(argv[1], NULL, 10));
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);  

	//SOCKET
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
		printf("ERROR opening socket");	

	//BIND
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		printf("ERROR on binding");
	
	clilen = sizeof(struct sockaddr_in);

	//RECVFROM
	n = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0) 
		printf("ERROR on recvfrom");
	printf("Received message from a client: %s\n", buf);

	//SENDTO
	n = sendto(sockfd, "ACKNOWLEDGED", 17, 0,(struct sockaddr *) &cli_addr, sizeof(struct sockaddr));
	if (n  < 0) 
		printf("ERROR on sendto");

	bzero(buf, 256);
	//GET USERID
	n = recvfrom(sockfd, buf, 256, 0, (struct sockaddr *) &cli_addr, &clilen);
	if (n < 0) 
		printf("ERROR on recvfrom");
	printf("Got userID: %s\n", buf);

	//ADD NEW CLIENT
	ClientList user_list = NULL;
	user_list = addClient(buf, sockfd, user_list);
	printUserList(user_list);

	//CLOSE
	close(sockfd);
	return 0;		
}



#endif