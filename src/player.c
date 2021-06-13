
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
#include "player.h"

typedef struct player {
	int rating;
	char *username;
	int ref;
	sem_t lock;
} PLAYER;

PLAYER *player_create(char *name){
	PLAYER *player = malloc(sizeof(PLAYER));
	if(player == NULL) return NULL;

	Sem_init(&(player->lock), 0, 1);
	P(&(player->lock));

	//PLAYER *player = &p;
	char *u_name = malloc(sizeof(char *)+1);
	if(u_name == NULL) return NULL;

	strcpy(u_name, name);
	player->username = u_name;
	player->ref = 1;
	player->rating = PLAYER_INITIAL_RATING;

	V(&(player->lock));

	return player;
}

PLAYER *player_ref(PLAYER *player, char *why){
	P(&(player->lock));
	player->ref = player->ref + 1;
	debug("%s", why);
	V(&(player->lock));
	return player;
}

void player_unref(PLAYER *player, char *why){
	P(&(player->lock));
	player->ref = player->ref - 1;
	debug("%s", why);

	if(player->ref == 0){
		free(player->username);
		free(player);
	}

	V(&(player->lock));
	return;
}

char *player_get_name(PLAYER *player){
	return player->username;
}

int player_get_rating(PLAYER *player){
	return player->rating;
}

void player_post_result(PLAYER *player1, PLAYER *player2, int result){
	P(&(player1->lock));
	P(&(player2->lock));

	double p1_round_score = 0.5;
	double p2_round_score = 0.5;

	double p1_rating = player1->rating;
	double p2_rating = player2->rating;

	if(result == 1){
		p1_round_score = 1.0;
		p2_round_score = 0.0;
	}
	else if(result == 2){
		p1_round_score = 0.0;
		p2_round_score = 1.0;
	}

	double e1 = 1.0 / (1.0 + pow(10.0,((double)(p2_rating - p1_rating)/400.0)));
	double e2 = 1.0 / (1.0 + pow(10.0,((double)(p1_rating - p2_rating)/400.0)));

	int r1 = p1_rating + (int)(32*(p1_round_score - e1));
	int r2 = p2_rating + (int)(32*(p2_round_score - e2));

	player1->rating = r1;
	player2->rating = r2;

	V(&(player1->lock));
	V(&(player2->lock));
	return;
}
