/*
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
#include "game.h"

typedef struct game{
	sem_t lock;
	int isOver;	//1 = game_over, 0 = in_progress
	int board[8];	//0 = empty, 1 = p1 (X), 2 = p2 (O)
	int ref;
	int winner; //0 = tie, 1 = p1, 2 = p2
}GAME;

typedef struct game_move{
	int position;
	int turn; //1 for p1, 2 for p2
}GAME_MOVE;

GAME *game_create(void){
	GAME *game = malloc(sizeof(GAME));
	Sem_init(&(game->lock), 0, 1);
	P(&(game->lock));

	game->isOver = 0;
	game->ref = 1;

	for(int x = 0; x < 8; x++){
		game->board[x] = 0;
	}

	V(&(game->lock));
	return game;
}

GAME *game_ref(GAME *game, char *why){
	P(&(game->lock));
	game->ref = game->ref + 1;
	debug("%s", why);
	V(&(game->lock));
	return game;
}

void game_unref(GAME *game, char *why){
	P(&(game->lock));
	game->ref = game->ref - 1;
	debug("%s", why);

	if(game->ref == 0){
		free(game);
	}
	V(&(game->lock));
	return;
}

int game_apply_move(GAME *game, GAME_MOVE *move){
	if(move->position < 1 || move->position > 9) return -1;
	if(game->board[move->position] != 0) return -1;

	P(&(game->lock));
	game->board[move->position] = move->turn;
	V(&(game->lock));
	return 0;
}

int game_resign(GAME *game, GAME_ROLE role){
	if(game_is_over(game) == 1) return -1;
	P(&(game->lock));

	//Set game to be over. and set winner
	game->isOver = 1;
	game->winner = 1;
	if(role == FIRST_PLAYER_ROLE) game->winner = 2;

	V(&(game->lock));
	return 0;

}


char *game_unparse_state(GAME *game){
	P(&(game->lock));
	char *game_board = malloc(sizeof(char *));
	int counter = 0;
	for(int x = 0; x < 8; x++){
		if(game->board[x] == 0) strcat(game_board, " ");
		else if(game->board[x] == 1) strcat(game_board, "X");
		else if(game_board[x] == 2) strcat(game_board, "O");
		if(counter == 2){
			strcat(game_board, "\n-----\n");
			counter = 0;
		}
		else{
			counter++;
			strcat(game_board, "|");
		}
	}
	V(&(game->lock));
	return game_board;
}

int game_is_over(GAME *game){
	return game->isOver;
}

GAME_ROLE game_get_winner(GAME *game){
	if(game->winner == 0 || game_is_over(game) == 0) return NULL_ROLE;
	else if(game-> winner == 1) return FIRST_PLAYER_ROLE;
	return SECOND_PLAYER_ROLE;
}

GAME_MOVE *game_parse_move(GAME *game, GAME_ROLE role, char *str){

}

char *game_unparse_move(GAME_MOVE *move){

}
*/