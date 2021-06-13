
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
#include "invitation.h"

typedef struct invitation {
	INVITATION_STATE invite_status;
	CLIENT *source;
	CLIENT *target;
	GAME_ROLE source_role;
	GAME_ROLE target_role;
	GAME *game;
	int ref;
	sem_t lock;
}INVITATION;

//sem_t lock;

INVITATION *inv_create(CLIENT *source, CLIENT *target, GAME_ROLE source_role, GAME_ROLE target_role){
	//INVITATION i;
	if(source == target) return NULL;

	INVITATION *invite = malloc(sizeof(INVITATION));
	if(invite == NULL) return NULL;
	Sem_init(&(invite->lock), 0, 1);

	P(&(invite->lock));
	invite->invite_status = INV_OPEN_STATE;
	invite->source = source;
	invite->target = target;
	invite->source_role = source_role;
	invite->target_role = target_role;
	invite->ref = 1;

	client_ref(source, "Source sent invite");
	client_ref(target, "target recieved invite");
	invite->game = NULL;
	V(&(invite->lock));
	return invite;
}

INVITATION *inv_ref(INVITATION *inv, char *why) {
	P(&(inv->lock));
	inv->ref = inv->ref + 1;
	debug("%s",why);
	V(&(inv->lock));
	return inv;
}

void inv_unref(INVITATION *inv, char *why){
	P(&(inv->lock));
	inv->ref = inv->ref - 1;
	debug("%s",why);
	if(inv->ref == 0){
		//free
		if(inv->game != NULL)
			game_unref(inv->game, "No invites left. Unref game and free invite");

	    client_unref(inv->source, "Source Invitation Being Discarded");
        client_unref(inv->target, "Invitation Being Discarded");

		free(inv);
	}
	V(&(inv->lock));

	return;
}

CLIENT *inv_get_source(INVITATION *inv){
	return inv->source;
}

CLIENT *inv_get_target(INVITATION *inv){
	return inv->target;
}

GAME_ROLE inv_get_source_role(INVITATION *inv){
	return inv->source_role;
}

GAME_ROLE inv_get_target_role(INVITATION *inv) {
	return inv->target_role;
}

GAME *inv_get_game(INVITATION *inv){
	if(inv->game != NULL)
		return inv->game;
	return NULL;
}

int inv_accept(INVITATION *inv){
	if(inv->invite_status == INV_OPEN_STATE) {
		P(&(inv->lock));
		inv->invite_status = INV_ACCEPTED_STATE;
		GAME *game = game_create();
		inv->game = game;
		game_ref(game, "game created");
		V(&(inv->lock));
		return 0;
	}
	else{
		return -1;
	}
}

int inv_close(INVITATION *inv, GAME_ROLE role){
	//If NULL_ROLE is passed, then the invitation can only be closed if there is no game in progress.

	if(role == NULL_ROLE){
		if(inv->game == NULL || game_is_over(inv->game) == 1){
			P(&(inv->lock));

			inv->invite_status = INV_CLOSED_STATE;
			V(&(inv->lock));
			return 0;
		}
		else{
			return -1;
		}
	}

	//Declined
	if(inv->invite_status == INV_OPEN_STATE && inv->game == NULL){
		P(&(inv->lock));
		inv->invite_status = INV_CLOSED_STATE;
		V(&(inv->lock));
		return 0;
	}
	//Player won
	else if(inv->invite_status == INV_ACCEPTED_STATE && game_is_over(inv->game) == 1){
		P(&(inv->lock));

		inv->invite_status = INV_CLOSED_STATE;
		V(&(inv->lock));
		return 0;
	}
	//Player left early
	else if(inv->invite_status == INV_ACCEPTED_STATE && game_is_over(inv->game) == 0){
		game_resign(inv->game, role);
		P(&(inv->lock));
		inv->invite_status = INV_CLOSED_STATE;
		V(&(inv->lock));

		game_get_winner(inv->game);
		return 0;
	}
	else{
		//V(&(inv->lock));
		return -1;
	}
	//return -1;
}
