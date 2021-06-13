
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

#include "debug.h"
#include "client_registry.h"
#include "client.h"
#include "protocol.h"
#include "server.h"
#include "player_registry.h"
#include "jeux_globals.h"
#include "csapp.h"
#include "server.h"

void *jeux_client_service(void *arg) {
	int connfd = *((int *)arg);
	free(arg);
	Pthread_detach(pthread_self());

	CLIENT *client = creg_register(client_registry, connfd);

	int is_loggedin = 0;
	while(1){
		JEUX_PACKET_HEADER *packet = malloc(sizeof(JEUX_PACKET_HEADER));
		void *recv_payload;
		int r = proto_recv_packet(connfd, packet, &recv_payload);
		if(r == -1){
			free(packet);
			debug("\n\n%s", "QUIT");
			break;
		}

		//Check for login
		if(is_loggedin == 0){
			if(packet->type == JEUX_LOGIN_PKT){
				PLAYER *player = preg_register(player_registry, recv_payload);
				if(player == NULL){
					free(recv_payload);
					if(client_send_nack(client) == -1) break;
				}
				else{
					free(recv_payload);
					int x = client_login(client, player);
					if(x == 0){
						//logged in. send ack packet
							player_unref(player, "login sent");

						is_loggedin = 1;
						if(client_send_ack(client, NULL, 0) == -1)break;
					}
					else{
						if(player != NULL)
							player_unref(player, "invalid login sent");

						//error. send NACK packer
						if(client_send_nack(client) == -1) break;
					}
				}
			}

			else{
				//Send NACK packet
				if(client_send_nack(client) == -1) break;
			}
		}
		//Already logged in. check for new input
		else{
			//Users already logged in
			if(packet->type == JEUX_LOGIN_PKT) client_send_nack(client);

			else if(packet->type == JEUX_USERS_PKT){
				PLAYER **p_list = creg_all_players(client_registry);
				char *user_list = (char *)calloc(sizeof(char*) * 10, 1);

				int num_players = 0;
				for(int x = 0; x < 200; x++){
					if(p_list[x] == NULL) break;

					num_players++;
					strcat(user_list, player_get_name(p_list[x]));
					strcat(user_list, "\t");
					sprintf(user_list + strlen(user_list), "%d", player_get_rating(p_list[x]));
					strcat(user_list, "\n");
					player_unref(p_list[x], "not needed");
					debug("%d", x);
					debug("%s", user_list);
				}
				debug("%s", user_list);
				if(client_send_ack(client, user_list, strlen(user_list)) == -1) break;
				free(p_list);
				free(user_list);
			}

			else if(packet->type == JEUX_INVITE_PKT){
				CLIENT *player2 = creg_lookup(client_registry, recv_payload);
				if(player2 == NULL) {
					free(recv_payload);
					client_send_nack(client);
				}
				else{
					int x = 0;
					//GAME_ROLE p1_role;
					if(packet->role == 1){
						x = client_make_invitation(client, player2, SECOND_PLAYER_ROLE, FIRST_PLAYER_ROLE);
						//client_unref(player2, "send invite");
					//	p1_role = SECOND_PLAYER_ROLE;
					}
					else if(packet->role == 2) {
						x = client_make_invitation(client, player2, FIRST_PLAYER_ROLE, SECOND_PLAYER_ROLE);
						//client_unref(player2, "after invite");
					//	p1_role = FIRST_PLAYER_ROLE;
					}

					if(x != -1){
						//send ack
						JEUX_PACKET_HEADER *p_header = malloc(sizeof(JEUX_PACKET_HEADER));
						p_header->id = x;
						p_header->type = JEUX_ACK_PKT;
						p_header->size = 0;
						//p_header->role = p1_role;
						client_unref(player2, "send invite");
						if(client_send_packet(client, p_header, 0) == -1)break;
						free(p_header);

					}
					else{
						client_unref(player2, "after invite");
						if(client_send_nack(client) == -1) {
						break;
						}
					}
				}
			}

			else if(packet->type == JEUX_REVOKE_PKT){
				int x = client_revoke_invitation(client, packet->id);
				if(x == 0){
					if(client_send_ack(client, NULL, 0) == -1);
				}
				else{
					if(client_send_nack(client) == -1) break;
				}
			}

			else if(packet->type == JEUX_DECLINE_PKT){
				int x = client_decline_invitation(client, packet->id);
				if(x == 0){
					if(client_send_ack(client, NULL, 0) == -1) break;
				}
				else{
					if(client_send_nack(client) == -1) break;
				}
			}

			else if(packet->type == JEUX_ACCEPT_PKT){
				char *turn;
				int x = client_accept_invitation(client, packet->id, &turn);
				if(x == -1){
					if(client_send_nack(client) == -1) break;
				}
				else{
					if(turn == NULL){
						if(client_send_ack(client, NULL, 0) == -1) break;
					}
					else{
						if(client_send_ack(client, turn, strlen(turn)) == -1) break;
					}
					free(turn);
				}
			}

			else if(packet->type == JEUX_MOVE_PKT){
				int x = client_make_move(client, packet->id, recv_payload);
				if(x == 0){
					client_send_ack(client, NULL, 0);
				}
				else{
					client_send_nack(client);
				}
			}

			else if(packet->type == JEUX_RESIGN_PKT){
				int x = client_resign_game(client, packet->id);
				if(x == 0){
					if(client_send_ack(client, NULL, 0) == -1) break;
				}
				else{
					if(client_send_nack(client) == -1) break;
				}
			}
		}
	}

	if(client == NULL){
		debug("%s", "finished loop");
	}

	//unregister client
	client_logout(client);
	creg_unregister(client_registry, client);
	//free(packet);

	//Close(connfd);

	return NULL;
}

