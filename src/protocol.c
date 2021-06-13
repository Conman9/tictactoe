
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "client_registry.h"
#include "player_registry.h"
#include "jeux_globals.h"
#include "csapp.h"

int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data){
	int anotha = ntohs(hdr->size);
	if(rio_writen(fd, hdr, sizeof(JEUX_PACKET_HEADER)) <= 0) return -1;
	if(anotha != 0){
		if(rio_writen(fd, data, anotha) <= 0) return -1;
	}
	return 0;
}

int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp){
	if(rio_readn(fd, hdr, sizeof(JEUX_PACKET_HEADER)) <= 0) return -1;
	int anotha = ntohs(hdr->size);
	//hdr->size = anotha;
	//hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
	//hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);
	//debug("%s", "recieving packet");
	if(anotha != 0){
		*payloadp = malloc(anotha);
		if(rio_readn(fd, *payloadp, anotha) <= 0) return -1;
	}
	//debug("%s", "packet recieved 1");
	return 0;
}
