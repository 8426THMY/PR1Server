#include "pr1_gameRespond.h"


#define PLAYER_INFO_BUFFER_SIZE (1 + ULONG_MAX_CHARS + PLAYER_NAME_MAX_LENGTH + FLOAT_MAX_LENGTH + (SHORT_MAX_CHARS * 6) + 1)
#define CHAT_MAX_MESSAGES 30


#include <stdlib.h>
#include <string.h>

#include "../server/socketTCP.h"

#include "pr1_util.h"
#include "pr1_race.h"


char *chatMessages[CHAT_MAX_MESSAGES];
size_t chatMessagesSize = 0;


void gameRespondLogin(socketServer *server, const size_t clientID, player *sender){
	//If the client hasn't already logged in, add them to our player list!
	if(sender == NULL){
		player tempPlayer;
		playerInit(&tempPlayer);

		playerUpdateInfo(&tempPlayer, server->buffer, clientID);
		vectorAdd(&playerList, &tempPlayer);

		sender = (player *)vectorGet(&playerList, playerList.size - 1);

	//Otherwise, simply update their information.
	}else{
		playerUpdateInfo(sender, server->buffer, clientID);
	}

	//If everything is valid, let them know that they're free to enter the lobby!
	if(playerValidateInfo(sender)){
		char enterLobby[1 + ULONG_MAX_CHARS + 1];
		enterLobby[0] = 'i';
		const size_t enterLobbyLength = 1 + ultostr(clientID, &enterLobby[1]);

		serverSendTCP(server, clientID, enterLobby, enterLobbyLength + 1);

	//Otherwise, just disconnect them.
	}else{
		serverDisconnectTCP(server, clientID);
	}
}

void gameRespondEnterLobby(const socketServer *server, player *sender){
	//If they're in a race, make them leave it!
	if(sender->raceID > 0){
		gameRespondLeaveRace(server, sender);
	}

	//Create a string containing the new client's information!
	char senderInfo[PLAYER_INFO_BUFFER_SIZE];
	const size_t senderInfoLength = sprintf(senderInfo, "p%u`%s`%f`%hu`%hu`%hu`%hu`%hu`%hu",
	                                        sender->id,    sender->name, sender->rank,
	                                        sender->head,  sender->body, sender->feet,
	                                        sender->speed, sender->jump, sender->traction);

	size_t i;
	for(i = 0; i < playerList.size; ++i){
		const player *currentPlayer = (player *)vectorGet(&playerList, i);

		//Send this player's information to everyone else!
		if(currentPlayer != sender){
			serverSendTCP(server, currentPlayer->id, senderInfo, senderInfoLength + 1);
		}

		//Create a string containing the current player's information!
		char playerInfo[PLAYER_INFO_BUFFER_SIZE];
		const size_t playerInfoLength = sprintf(playerInfo, "p%u`%s`%f`%hu`%hu`%hu`%hu`%hu`%hu",
		                                        currentPlayer->id,    currentPlayer->name, currentPlayer->rank,
		                                        currentPlayer->head,  currentPlayer->body, currentPlayer->feet,
		                                        currentPlayer->speed, currentPlayer->jump, currentPlayer->traction);
		//Now send it to the connecting client!
		serverSendTCP(server, sender->id, playerInfo, playerInfoLength + 1);


		//Also send the connecting client which race slots everyone else is occupying!
		if(currentPlayer->raceNum > 0 && currentPlayer->slotNum > 0){
			char playerSlot[1 + ULONG_MAX_CHARS + 1 + ULONG_MAX_CHARS + 1 + ULONG_MAX_CHARS + 1];
			const size_t playerSlotLength = sprintf(playerSlot, "j%u`%u`%u", currentPlayer->raceNum, currentPlayer->slotNum, currentPlayer->id);
			serverSendTCP(server, sender->id, playerSlot, playerSlotLength + 1);

			//If a player is ready, let the connecting client know!
			if(raceList[currentPlayer->raceNum - 1].slotState[currentPlayer->slotNum - 1] == 1){
				char playerReady[1 + ULONG_MAX_CHARS + 1];
				playerReady[0] = 'r';
				size_t playerReadyLength = 1 + ultostr(currentPlayer->id, &playerReady[1]);
				serverSendTCP(server, sender->id, playerReady, playerReadyLength + 1);
			}
		}
	}

	//Send the user the message of the day!
	//std::string motdData = "^0`&#0;`" + motd + "\n";
	//serverSendTCP(server, sender->id, motdString, motdStringLength + 1);

	//Finally, send them the most recent chat messages!
	for(i = 0; i < chatMessagesSize; ++i){
		serverSendTCP(server, sender->id, chatMessages[i], strlen(chatMessages[i]) + 1);
	}
}

void gameRespondSendChatMessage(const socketServer *server, const player *sender){
	printf("%s: %s\n", sender->name, &server->buffer[1]);

	char senderIDString[ULONG_MAX_CHARS + 1];
	const size_t senderIDStringLength = ultostr(sender->id, senderIDString);

	//Format the message!
	const size_t messageLength = sender->id + 1 + sender->nameLength + 1 + (server->bufferLength - 1);
	char *message = malloc(messageLength + 1);
	message[0] = '^';
	memcpy(message + 1, senderIDString, senderIDStringLength);
	message[1 + senderIDStringLength] = '`';
	memcpy(message + 1 + senderIDStringLength + 1, sender->name, sender->nameLength);
	message[1 + senderIDStringLength + 1 + sender->nameLength] = '`';
	strcpy(message + (messageLength - (server->bufferLength - 2)), &server->buffer[1]);
	message[messageLength] = '\0';

	size_t i;
	//Send it to everyone!
	for(i = 0; i < playerList.size; ++i){
		serverSendTCP(server, ((player *)vectorGet(&playerList, i))->id, message, messageLength + 1);
	}

	//We don't want to keep too many messages.
	if(chatMessagesSize >= CHAT_MAX_MESSAGES){
		free(chatMessages[0]);
		memmove(&chatMessages[1], chatMessages, chatMessagesSize - 1);
		--chatMessagesSize;
	}
	chatMessages[chatMessagesSize] = message;
	++chatMessagesSize;
}

void gameRespondJoinRace(const socketServer *server, player *sender){
	const size_t oldRaceNum = sender->raceNum;

	//Retrieve the new race and slot numbers from the buffer!
	size_t newRaceNum;
	size_t newSlotNum;
	char *endPos = strchr(&server->buffer[1], '`');
	if(endPos != NULL){
		newRaceNum = strtoul(&server->buffer[1], &endPos, 10);
		newSlotNum = strtoul(endPos + 1, NULL, 10);
	}else{
		newRaceNum = 0;
		newSlotNum = 0;
	}

	unsigned char success = 0;
	//If the player is leaving, free them from the slot!
	if(newRaceNum <= 0 || newSlotNum <= 0){
		raceLeaveSlot(&raceList[oldRaceNum - 1], sender->slotNum - 1, sender->id);
		sender->raceNum = 0;
		sender->slotNum = 0;

		success = 1;

	//Otherwise, if they're joining an empty slot, add them to it!
	}else if(raceJoinSlot(&raceList[newRaceNum - 1], newSlotNum - 1, sender->id)){
		raceLeaveSlot(&raceList[oldRaceNum - 1], sender->slotNum - 1, sender->id);
		sender->raceNum = newRaceNum;
		sender->slotNum = newSlotNum;

		success = 1;
	}

	//If the user was able to join or leave a slot, let everyone else know!
	if(success){
		//Format the response!
		char *raceSlotString = malloc((server->bufferLength - 1) + 1 + ULONG_MAX_CHARS + 1);
		strcpy(raceSlotString, server->buffer);
		raceSlotString[server->bufferLength - 1] = '`';
		const size_t raceSlotStringLength = (server->bufferLength - 1) + 1 + ultostr(sender->id, &raceSlotString[server->bufferLength]);

		size_t i;
		//Send it to everyone!
		for(i = 0; i < playerList.size; ++i){
			serverSendTCP(server, ((player *)vectorGet(&playerList, i))->id, raceSlotString, raceSlotStringLength + 1);
		}

		free(raceSlotString);
	}

	//If the race that the player was previously in is ready, start it!
	if(oldRaceNum > 0 && raceIsReady(&raceList[oldRaceNum - 1])){
		gameRespondStartRace(server, oldRaceNum);
	}
}

void gameRespondReadyUp(const socketServer *server, const player *sender){
	//If the user is actually in a race slot, change their state!
	if(sender->raceNum > 0 && sender->slotNum > 0){
		race *r = &raceList[sender->raceNum - 1];
		raceReadyUp(r, sender->slotNum - 1, sender->id);

		//If everyone else is ready, start the race!
		if(raceIsReady(r)){
			gameRespondStartRace(server, sender->raceNum);

		//Otherwise, just tell everyone that they're ready!
		}else{
			//Format the response!
			char *readyString = malloc((server->bufferLength - 1) + ULONG_MAX_CHARS + 1);
			strcpy(readyString, server->buffer);
			const size_t readyStringLength = (server->bufferLength - 1) + ultostr(sender->id, &readyString[server->bufferLength - 1]);

			size_t i;
			//Send it to everyone!
			for(i = 0; i < playerList.size; ++i){
				serverSendTCP(server, ((player *)vectorGet(&playerList, i))->id, readyString, readyStringLength + 1);
			}
		}
	}
}

void gameRespondRankUp(const socketServer *server, player *sender){
	raceInstance *senderRace = (raceInstance *)vectorGet(&currentRaces, sender->raceID - 1);

	//Acknowledge that the user has finished the race!
	++senderRace->playersFinished;
	//Calculate their new rank ourselves, just in case they've done something funny!
	sender->rank = raceInstCalculateRank(senderRace, sender->rank);

	//Create a string containing the client's information!
	char senderInfo[PLAYER_INFO_BUFFER_SIZE];
	const size_t senderInfoLength = sprintf(senderInfo, "p%u`%s`%f`%hu`%hu`%hu`%hu`%hu`%hu",
	                                        sender->id,    sender->name, sender->rank,
	                                        sender->head,  sender->body, sender->feet,
	                                        sender->speed, sender->jump, sender->traction);

	size_t i;
	//Send it to everyone!
	for(i = 0; i < playerList.size; ++i){
		serverSendTCP(server, ((player *)vectorGet(&playerList, i))->id, senderInfo, senderInfoLength + 1);
	}
}

void gameRespondPlayRace(const socketServer *server, const player *sender){
	/*
	't' - The client has pressed a movement key.
	'q' - The client is sending an update on their position.
	'k' - The client has got an item.
	*/

	size_t i;
	//Echo the message to all the other clients in this race!
	for(i = 0; i < RACE_NUM_SLOTS; ++i){
		size_t currentSlotID = ((raceInstance *)vectorGet(&currentRaces, sender->raceID - 1))->playerIDs[i];
		if(currentSlotID != 0 && currentSlotID != sender->id){
			serverSendTCP(server, currentSlotID, &server->buffer[1], server->bufferLength - 1);
		}
	}
}

void gameRespondStartRace(const socketServer *server, size_t raceNum){
	size_t newRaceID = 0;
	raceInstance *newRace;

	size_t i;
	//Check if there are any empty race instances we can use!
	for(i = 0; i < currentRaces.size; ++i){
		newRace = (raceInstance *)vectorGet(&currentRaces, i);
		if(raceInstEmpty(newRace)){
			raceInstInit(newRace);
			newRace->map = raceNum;
			newRaceID = i + 1;

			break;
		}
	}
	//If there weren't any, create a new one!
	if(newRaceID == 0){
		raceInstance newRaceInst;
		raceInstInit(&newRaceInst);
		newRaceInst.map = raceNum;
		vectorAdd(&currentRaces, &newRaceInst);

		newRaceID = currentRaces.size;
		newRace = (raceInstance *)vectorGet(&currentRaces, newRaceID - 1);
	}

	char raceNumString[ULONG_MAX_CHARS + 1];
	const size_t raceNumStringLength = ultostr(raceNum, raceNumString);

	char sendToRace[1 + ULONG_MAX_CHARS + 1];
	sendToRace[0] = 'm';
	strcpy(&sendToRace[1], raceNumString);

	char clearSlots[1 + ULONG_MAX_CHARS + 1];
	clearSlots[0] = 'z';
	strcpy(&clearSlots[1], raceNumString);

	//Let everyone who was in the race play it and clear the slots for everyone else!
	for(i = 0; i < playerList.size; ++i){
		player *currentPlayer = (player *)vectorGet(&playerList, i);

		//If the user is in this race, send them to it!
		if(currentPlayer->raceNum == raceNum){
			//Add the player to the race instance!
			raceInstAddPlayer(newRace, currentPlayer->slotNum - 1, currentPlayer->id);
			currentPlayer->raceID = newRaceID;

			//Clear the race's slots for the server!
			raceLeaveSlot(&raceList[currentPlayer->raceNum - 1], currentPlayer->slotNum - 1, currentPlayer->id);

			serverSendTCP(server, currentPlayer->id, sendToRace, 1 + raceNumStringLength + 1);

		//Otherwise, clear the race's slots for the user!
		}else{
			serverSendTCP(server, currentPlayer->id, clearSlots, 1 + raceNumStringLength + 1);
		}
	}
}

void gameRespondLeaveRace(const socketServer *server, player *sender){
	raceInstance *senderRace = (raceInstance *)vectorGet(&currentRaces, sender->raceID - 1);

	//Remove the player from the race!
	raceInstRemovePlayer(senderRace, sender->slotNum - 1, sender->id);

	size_t i;
	//If the race is now empty and it's the last race instance in the vector, loop through all the
	//instances starting from the end and delete all empty ones until we get to one that isn't empty!
	if(raceInstEmpty(senderRace)){
		if(sender->raceID == currentRaces.size){
			i = currentRaces.size - 1;
			vectorRemove(&currentRaces, i);

			while(i > 0){
				--i;
				if(raceInstEmpty((raceInstance *)vectorGet(&currentRaces, i))){
					vectorRemove(&currentRaces, i);
				}else{
					break;
				}
			}
		}

	//Otherwise, if the race isn't empty, tell everyone in the race that the user has left!
	}else{
		char removeFromRace[1 + ULONG_MAX_CHARS + 1];
		removeFromRace[0] = 's';
		const size_t removeFromRaceLength = 1 + ultostr(sender->id, &removeFromRace[1]);

		for(i = 0; i < RACE_NUM_SLOTS; ++i){
			size_t currentSlotID = senderRace->playerIDs[i];
			if(currentSlotID != 0){
				serverSendTCP(server, currentSlotID, removeFromRace, removeFromRaceLength + 1);
			}
		}
	}

	//Don't forget to reset these so we don't think they're still in the race!
	sender->raceNum = 0;
	sender->slotNum = 0;
	sender->raceID = 0;
}

void gameRespondFinishRace(const socketServer *server, const player *sender){
	const raceInstance *senderRace = (raceInstance *)vectorGet(&currentRaces, sender->raceID - 1);

	size_t i;
	//Echo the message to everyone in this race!
	for(i = 0; i < RACE_NUM_SLOTS; ++i){
		size_t currentSlotID = senderRace->playerIDs[i];
		if(currentSlotID != 0){
			serverSendTCP(server, currentSlotID, &server->buffer[1], server->bufferLength - 1);
		}
	}
}

void gameRespondDomainPolicy(const socketServer *server, const size_t clientID){
	serverSendTCP(server, clientID, "<?xml version=\"1.0\"?><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\"/></cross-domain-policy>", 109);
}