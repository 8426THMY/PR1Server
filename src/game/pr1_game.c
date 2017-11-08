#include "pr1_game.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../server/socketTCP.h"

#include "pr1_util.h"
#include "pr1_race.h"
#include "pr1_player.h"
#include "pr1_gameRespond.h"


//Forward-declare our helper functions!
static void gameBuffer(socketServer *server, const size_t clientID);
static void gameDisconnect(socketServer *server, const size_t clientID);


void gameInit(){
	size_t i;
	for(i = 0; i < RACE_NUM_MAPS; ++i){
		raceInit(&raceList[i]);
	}

	vectorInit(&currentRaces, sizeof(raceInstance));
	vectorInit(&playerList, sizeof(player));
}

unsigned char gameLoadServer(socketServer *server, const int type, const int protocol){
	char *ip = NULL;
	size_t ipLength = 0;
	unsigned short port = DEFAULT_PORT;
	size_t bufferSize = DEFAULT_BUFFER_SIZE;
	loadConfig("./config/gameServer.cfg", &ip, &ipLength, &port, &bufferSize);

	const unsigned char success = serverInit(server, type, protocol, ip, ipLength, port, bufferSize, &gameBuffer, &gameDisconnect);

	free(ip);
	return(success);
}

void gameClose(){
	vectorClear(&currentRaces);

	size_t i;
	for(i = 0; i < playerList.size; ++i){
		playerRemove((player *)vectorGet(&playerList, i));
	}
	vectorClear(&playerList);
}


static void gameBuffer(socketServer *server, const size_t clientID){
	unsigned char success = 0;


	player *sender = playerFind(clientID);
	//The client has just connected or changed their loadout!
	if(server->buffer[0] == 'n'){
		gameRespondLogin(server, clientID, sender);

		success = 1;

	//Make sure the client is actually logged in!
	}else if(sender != NULL){
		//The client has entered the lobby!
		if(server->buffer[0] == 'o'){
			gameRespondEnterLobby(server, sender);

			success = 1;

		//The client has sent a chat message!
		}else if(server->buffer[0] == '^'){
			gameRespondSendChatMessage(server, sender);

			success = 1;

		//The client wants to join a race!
		}else if(server->buffer[0] == 'j'){
			gameRespondJoinRace(server, sender);

			success = 1;

		//The client is ready to start a race!
		}else if(server->buffer[0] == 'r'){
			gameRespondReadyUp(server, sender);

			success = 1;

		//The client ranked up!
		}else if(server->buffer[0] == 'b'){
			gameRespondRankUp(server, sender);

			success = 1;

		//The client is sending some information related to a race!
		}else if(server->buffer[0] == '#'){
			//The client is leaving a race!
			if(server->buffer[1] == 's'){
				gameRespondLeaveRace(server, sender);

				success = 1;

			//Otherwise, we pretty much just relay it to the other clients.
			}else{
				gameRespondPlayRace(server, sender);

				success = 1;
			}

		//The client has finished a race!
		}else if(server->buffer[0] == '%' && server->buffer[1] == 'f'){
			gameRespondFinishRace(server, sender);

			success = 1;

		//The client has sent us a ping!
		}else if(server->buffer[0] == 'a'){
			success = 1;
		}
	}else{
		//If the client is not logged in, check if they're requesting the policy file.
		if(memcmp(server->buffer, "<policy-file-request/>", sizeof("<policy-file-request/>") - 1) == 0){
			gameRespondDomainPolicy(server, clientID);

			success = 1;

		//If they aren't, disconnect them.
		}else{
			printf("Client #%u tried to send something without logging in:\n"
			       "%s\n", clientID, server->buffer);
			serverDisconnectTCP(server, clientID);

			success = 1;
		}
	}


	//Looks like the client has sent something we don't recognize!
	if(!success){
		printf("Client #%u has sent something we can't handle at the moment:\n"
		       "%s\n\n", clientID, server->buffer);
	}
}

static void gameDisconnect(socketServer *server, const size_t clientID){
	const size_t senderPos = playerFindID(clientID);
	player *sender = (player *)vectorGet(&playerList, senderPos);


	//If the client is logged in, remove them from any races or race slots!
	if(sender != NULL){
		//Convert their I.D. to a string.
		char senderID[ULONG_MAX_CHARS + 1];
		ultostr(clientID, senderID);

		//Generate a message to tell the other clients that this one has left the server.
		const size_t removeFromListLength = 1 + ULONG_MAX_CHARS + 1;
		char removeFromList[removeFromListLength];
		removeFromList[0] = 'd';
		strcpy(&removeFromList[1], senderID);

		size_t i;
		//If the player is occupying a race slot, tell everyone
		//else to remove them from it as well as the player list!
		if(sender->raceNum > 0 && sender->slotNum > 0){
			//Generate a message to tell the other clients that this one has left the race slot.
			const size_t removeFromSlotLength = 11 + ULONG_MAX_CHARS + 1;
			char removeFromSlot[removeFromSlotLength];
			memcpy(removeFromSlot, "jnone`none`", 11);
			strcpy(&removeFromSlot[11], senderID);

			for(i = 0; i < playerList.size; ++i){
				size_t currentPlayerID = ((player *)vectorGet(&playerList, i))->id;
				serverSendTCP(server, currentPlayerID, removeFromSlot, removeFromSlotLength);
				serverSendTCP(server, currentPlayerID, removeFromList, removeFromListLength);
			}

			//Remove them from the race slot on our side!
			race *oldRace = &raceList[sender->raceNum - 1];
			raceLeaveSlot(oldRace, sender->slotNum - 1, sender->id);
			//If the race is ready to start, start it!
			if(raceIsReady(oldRace)){
				gameRespondStartRace(server, sender->raceNum);
			}

		//Otherwise, just remove them from the player list.
		}else{
			for(i = 0; i < playerList.size; ++i){
				serverSendTCP(server, ((player *)vectorGet(&playerList, i))->id, removeFromList, removeFromListLength);
			}
		}

		//If the player was in a race, remove them from it!
		if(sender->raceID > 0){
			gameRespondLeaveRace(server, sender);
		}


		//Remove the player from our player list.
		playerRemove(sender);
		vectorRemove(&playerList, senderPos);
	}


	printf("Client #%u has been disconnected from the game server.\n", clientID);
}