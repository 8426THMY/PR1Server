#include "pr1_player.h"


#define NUM_HEADS       11
#define NUM_BODIES      11
#define NUM_FEET        11
#define STAT_MAX        100
#define NUM_POINTS      150


#include <string.h>

#include "pr1_util.h"


vector playerList;


void playerInit(player *p){
	memset(p, 0, sizeof(*p));
}


void playerUpdateInfo(player *p, const char *buffer, const size_t clientID){
	//Save their I.D. on the off-chance that it's changed.
	p->id = clientID;

	//Find the first grave accent, which marks the end of the player's name.
	char *tokPos = strchr(buffer + 1, '`');

	//Save the player's name.
	p->nameLength = tokPos - (buffer + 1);
	p->name = realloc(p->name, p->nameLength + 1);
	memcpy(p->name, buffer + 1, p->nameLength);
	p->name[p->nameLength] = '\0';

	//Save the rest of the player's information.
	p->rank     = strtod(++tokPos, &tokPos);
	p->head     = strtoul(++tokPos, &tokPos, 10);
	p->body     = strtoul(++tokPos, &tokPos, 10);
	p->feet     = strtoul(++tokPos, &tokPos, 10);
	p->speed    = strtoul(++tokPos, &tokPos, 10);
	p->jump     = strtoul(++tokPos, &tokPos, 10);
	p->traction = strtoul(++tokPos, &tokPos, 10);
}

//Check if a player's information is valid.
unsigned char playerValidateInfo(player *p){
	//If their parts and stats aren't valid, return 0.
	if(p->head < 1 || p->head > NUM_HEADS || p->body < 1 || p->body > NUM_BODIES || p->feet < 1 || p->feet > NUM_FEET ||
	   p->speed > STAT_MAX || p->jump > STAT_MAX || p->traction > STAT_MAX || (p->speed + p->jump + p->traction) > NUM_POINTS){

		return(0);
	}

	//If the player's username's size is invalid or it contains bad characters, change it to something more acceptable!
	if(p->nameLength <= 0 || p->nameLength > PLAYER_NAME_MAX_LENGTH || strchr(p->name, '`') != NULL || strchr(p->name, '<') != NULL){
		char idString[ULONG_MAX_CHARS + 1];
		size_t idStringLength = ultostr(p->id, idString);

		p->nameLength = 8 + idStringLength;
		p->name = realloc(p->name, 8 + idStringLength + 1);
		memcpy(p->name, "Player #", 8);
		memcpy(p->name + 8, idString, idStringLength);
		p->name[p->nameLength] = '\0';

	//Otherwise, if they trying to use the "ghost string", change it to something else!
	}else if(memcmp(p->name, "#&0;", 5) == 0){
		char idString[ULONG_MAX_CHARS + 1];
		size_t idStringLength = ultostr(p->id, idString);

		p->nameLength = 7 + idStringLength;
		p->name = realloc(p->name, 7 + idStringLength + 1);
		memcpy(p->name, "Ghost #", 7);
		memcpy(p->name + 7, idString, idStringLength);
		p->name[p->nameLength] = '\0';
	}


	return(1);
}

//Find a player and return a pointer to their structure.
player *playerFind(const size_t clientID){
	size_t i;
	for(i = 0; i < playerList.size; ++i){
		player *sender = (player *)vectorGet(&playerList, i);
		if(sender->id == clientID){
			return(sender);
		}
	}

	return(NULL);
}

//Find a player and return their position in the vector.
size_t playerFindID(const size_t clientID){
	size_t i;
	for(i = 0; i < playerList.size; ++i){
		if(((player *)vectorGet(&playerList, i))->id == clientID){
			break;
		}
	}

	return(i);
}


void playerRemove(player *p){
	if(p->name != NULL){
		free(p->name);
	}
}