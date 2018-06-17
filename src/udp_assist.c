#ifndef UDP_ASSIST_CODE
#define UDP_ASSIST_CODE

#include "udp_assist.h"


int send_packet(int *msgid, char *buffer, int sockid, struct sockaddr_in *to){
    Frame packet;
    int status = 0;

	socklen_t tolen = sizeof(struct sockaddr_in);

    memcpy(packet.buffer, buffer, BUFFER_SIZE);
    packet.ack = FALSE;
    packet.message_id = *msgid;

    while(packet.ack != TRUE) {

	   	status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) to, sizeof(struct sockaddr_in));
	   	if (status < 0) {
		    printf("\n[ERROR sending packet]: Sending packet fault\n");
            return -1;
	    }
;
        status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) to, &tolen);
        if (status < 0) {
		    printf("\n[ERROR sending packet]: Receivig ack fault\n");
            return -2;
	    }

	}

    *msgid = *msgid + 1;
    return 0;
}


int recv_packet(int *msgid, char *buffer, int sockid, struct sockaddr_in *from){
    Frame packet;
    int status = 0;
		
	socklen_t fromlen = sizeof(struct sockaddr_in);

    do {
        
		status = recvfrom(sockid, &packet, sizeof(packet), 0, (struct sockaddr *) from, &fromlen);
		if (status < 0) {
		    printf("\n[ERROR receiving packet]: Receiving packet fault\n");
            return -1;
	    }
        /* Check if it's the awaited message*/
        if(packet.message_id == *msgid)
            packet.ack = TRUE;

		status = sendto(sockid, &packet, sizeof(packet), 0, (const struct sockaddr *) from, sizeof(struct sockaddr_in));
        if (status < 0) {
		    printf("\n[ERROR receiving packet]: Sending ack fault\n");
            return -1;
	    }

	}while (packet.ack != TRUE );

    *msgid = *msgid + 1;
    memcpy(buffer, packet.buffer, BUFFER_SIZE);
    return 0;
}




#endif