
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
#include "player.h"

typedef struct player_registry {
	int num_players;
	PLAYER *players[200];
	sem_t lock;

}PLAYER_REGISTRY;

PLAYER_REGISTRY *preg_init(void){
	PLAYER_REGISTRY *pr = malloc(sizeof(PLAYER_REGISTRY) * 200);
	if(pr == NULL) return NULL;

	Sem_init(&(pr->lock), 0, 1);

	P(&(pr->lock));
	pr->num_players = 0;

	for(int x = 0; x < 200; x++){
		pr->players[x] = NULL;
	}

	V(&(pr->lock));
	return pr;
}

void preg_fini(PLAYER_REGISTRY *preg){
	for(int x = 0; x < 200; x++){
		if(preg->players[x] == NULL) break;

		free(preg->players[x]);
	}
	free(preg);
	return;
}

PLAYER *preg_register(PLAYER_REGISTRY *preg, char *name){
	if(name == NULL) return NULL;

	P(&(preg->lock));

	int position = 0;
	//Check if name already exists
	for(int x = 0; x < 200; x++){
		if(preg->players[x] == NULL){ position = x; break; }

		//if name already exists
		if(!strcmp(player_get_name(preg->players[x]), name)){
			player_ref(preg->players[x], "Name already exists");
			V(&(preg->lock));
			return preg->players[x];
		}
	}

	PLAYER *p = player_create(name);
	if(p == NULL) return NULL;

	player_ref(p, "Adding new player to player registry");
	preg->players[position] = p;

	V(&(preg->lock));
	return p;
}
