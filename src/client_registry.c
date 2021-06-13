
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
#include <semaphore.h>

#include "client_registry.h"
#include "client.h"
#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "player_registry.h"
#include "jeux_globals.h"
#include "csapp.h"

sem_t wait_empty;

typedef struct client_registry {
	int num_clients;
	CLIENT *clients[MAX_CLIENTS];
	sem_t lock;
} CLIENT_REGISTRY;


//Initializes the registry struct
CLIENT_REGISTRY *creg_init(){

	CLIENT_REGISTRY *cr = malloc(sizeof(CLIENT_REGISTRY) * MAX_CLIENTS);
	//CLIENT_REGISTRY *cr = &c;
	Sem_init(&wait_empty, 0, 1);
	Sem_init(&(cr->lock), 0, 1);

	P(&(cr->lock));
	cr->num_clients = 0;

	for(int x = 0; x < MAX_CLIENTS; x++){
		cr->clients[x] = NULL;
	}

	V(&(cr->lock));
	return cr;
}

void creg_fini(CLIENT_REGISTRY *cr){

	//if(cr->num_clients == 0)
	free(cr);

	return;
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd){
	P(&(cr->lock));
	if(cr->num_clients == MAX_CLIENTS) {
		V(&(cr->lock));
		return NULL;
	}

	CLIENT *client = client_create(cr, fd);
	//cr->clients[cr->num_clients] = client;
	for(int x = 0; x < MAX_CLIENTS; x++){
		if(cr->clients[x] == NULL) { debug("%s\n", "reg_new_client");cr->clients[x] = client; break;}
	}

	cr->num_clients = cr->num_clients + 1;

	V(&(cr->lock));
	return client;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client){
	P(&(cr->lock));

	int position = -1;
	for(int x = 0; x < MAX_CLIENTS; x++){
		if(cr->clients[x] == client){
			position = x;
			client_unref(client, "unregistering client from registry");
			cr->num_clients = cr->num_clients - 1;
		}
	}

	if(position == -1){
		V(&(cr->lock));
		return -1;
	}

	for(int x = position; x < MAX_CLIENTS-1; x++){
		cr->clients[x] = cr->clients[x+1];
		if(cr->clients[x] == NULL) break;
	}

	//any threads that were waiting can continue
	if(cr->num_clients == 0){
		//creg_fini(cr);
		V(&wait_empty);
	}

	V(&(cr->lock));
	return 0;
}

// Find client with same username
CLIENT *creg_lookup(CLIENT_REGISTRY *cr, char *user){
	P(&(cr->lock));
	for(int x = 0; x < cr->num_clients; x++){
		if(cr->clients[x] == NULL) break;

		if(!strcmp(player_get_name(client_get_player(cr->clients[x])), user)){
			client_ref(cr->clients[x], "Found user in search");
			V(&(cr->lock));
			return cr->clients[x];
		}
	}
	V(&(cr->lock));
	return NULL;
}

PLAYER **creg_all_players(CLIENT_REGISTRY *cr){
	P(&(cr->lock));

	PLAYER **list_players = malloc(sizeof(PLAYER *) * ((cr->num_clients)+1));

	int c = 0;
	for(int x = 0; x < cr->num_clients; x++){
		if(cr->clients[x] == NULL) break;
		PLAYER *p = client_get_player(cr->clients[x]);
		if(p != NULL){
			//client_ref(cr->clients[x], "client added to players list");
			player_ref(p, "player added to players list");
			list_players[c] = p;
			//memcpy()
			c++;
		}
	}

	list_players[c] = NULL;
	V(&(cr->lock));
	return list_players;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr){
	//if(wait_empty == 1)
	P(&wait_empty);
	return;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr){
	for(int x = 0; x < cr->num_clients; x++){
		shutdown(client_get_fd(cr->clients[x]), SHUT_RD);
	}
	return;
}
