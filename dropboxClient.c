#ifndef CLIENT_CODE
#define CLIENT_CODE


//código exemplo que o sor colocou no moodle

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PORT 4000

int main(int argc, char *argv[])
{
    int sockfd, n;
	unsigned int length;
	struct sockaddr_in serv_addr, from;
	struct hostent *server;
	
	char buffer[256];
	if (argc < 2) {
		fprintf(stderr, "usage %s hostname\n", argv[0]);
		exit(0);

	}
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }	
	
    //SOCKET
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr_list[0]); //h_addr -> h_addr_list[0]
	bzero(&(serv_addr.sin_zero), 8);  

	printf("Enter the message: ");
	bzero(buffer, 256);
	fgets(buffer, 256, stdin);

    //SENDTO
	n = sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
	if (n < 0) 
		printf("ERROR sendto");
	
	length = sizeof(struct sockaddr_in);
	//RECVFROM
    n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &from, &length);
	if (n < 0)
		printf("ERROR recvfrom");

	printf("Got an ack: %s\n", buffer);
	
    //CLOSE
	close(sockfd);
	return 0;
}

#endif