#include "pr1_race.h"


#define MAP_NEWBIELAND     1
#define MAPDIF_NEWBIELAND  1.f

#define MAP_BUTO           2
#define MAPDIF_BUTO        3.f

#define MAP_PYRAMIDS       3
#define MAPDIF_PYRAMIDS    5.f

#define MAP_ROBOCITY       4
#define MAPDIF_ROBOCITY    2.f

#define MAP_ASSEMBLY       5
#define MAPDIF_ASSEMBLY    5.f

#define MAP_INFERNALHOP    6
#define MAPDIF_INFERNALHOP 8.f

#define MAP_GOINGDOWN      7
#define MAPDIF_GOINGDOWN   3.f

#define MAP_SLIP           8
#define MAPDIF_SLIP        7.f


race raceList[RACE_NUM_MAPS];
vector currentRaces;


void raceInit(race *r){
	memset(r, 0, sizeof(*r));
}

void raceInstInit(raceInstance *rInst){
	memset(rInst, 0, sizeof(*rInst));
}


//Let a player join a race slot.
unsigned char raceJoinSlot(race *r, const size_t slot, const size_t clientID){
	if(slot < RACE_NUM_SLOTS && r->slotID[slot] == 0){
		r->slotID[slot] = clientID;

		return(1);
	}else{
		return(0);
	}
}

//Remove a player from a race slot.
unsigned char raceLeaveSlot(race *r, const size_t slot, const size_t clientID){
	if(slot < RACE_NUM_SLOTS && r->slotID[slot] == clientID){
		r->slotID[slot] = 0;
		r->slotState[slot] = 0;

		return(1);
	}

	return(0);
}

//Ready up a player.
unsigned char raceReadyUp(race *r, const size_t slot, const size_t clientID){
	if(slot < RACE_NUM_SLOTS && r->slotID[slot] == clientID){
		r->slotState[slot] = 1;

		return(1);
	}

	return(0);
}


//Add a player to a race.
unsigned char raceInstAddPlayer(raceInstance *rInst, const size_t slot, const size_t clientID){
	if(slot < RACE_NUM_SLOTS && rInst->playerIDs[slot] == 0){
		rInst->playerIDs[slot] = clientID;
		++rInst->totalPlayers;

		return(1);
	}

	return(0);
}

//Remove a player from a race.
unsigned char raceInstRemovePlayer(raceInstance *rInst, const size_t slot, const size_t clientID){
	if(slot < RACE_NUM_SLOTS && rInst->playerIDs[slot] == clientID){
		rInst->playerIDs[slot] = 0;

		return(1);
	}

	return(0);
}

//Verify that the player's new rank was calculated correctly.
float raceInstCalculateRank(const raceInstance *rInst, const float oldRank){
	float mapDif;
	switch(rInst->map){
		case(MAP_NEWBIELAND):
			mapDif = MAPDIF_NEWBIELAND;
		break;
		case(MAP_BUTO):
			mapDif = MAPDIF_BUTO;
		break;
		case(MAP_PYRAMIDS):
			mapDif = MAPDIF_PYRAMIDS;
		break;
		case(MAP_ROBOCITY):
			mapDif = MAPDIF_ROBOCITY;
		break;
		case(MAP_ASSEMBLY):
			mapDif = MAPDIF_ASSEMBLY;
		break;
		case(MAP_INFERNALHOP):
			mapDif = MAPDIF_INFERNALHOP;
		break;
		case(MAP_GOINGDOWN):
			mapDif = MAPDIF_GOINGDOWN;
		break;
		case(MAP_SLIP):
			mapDif = MAPDIF_SLIP;
		break;
		default:
			mapDif = 0;
		break;
	}
	//"1 << rInst->playersFinished" computes 2 to the power of "rInst->playersFinished".
	float totalExp = (float)rInst->totalPlayers / (1 << rInst->playersFinished) * mapDif;

	return(oldRank + totalExp);
}


//Check if all the clients in the race are ready.
unsigned char raceIsReady(const race *r){
	size_t i;
	for(i = 0; i < RACE_NUM_SLOTS; ++i){
		//If there's a client in this slot who isn't ready, return 0.
		if(r->slotID[i] != 0 && r->slotState[i] == 0){
			return(0);
		}
	}

	return(1);
}

//Check if everyone has left the race.
unsigned char raceInstEmpty(raceInstance *rInst){
	size_t i;
	for(i = 0; i < RACE_NUM_SLOTS; ++i){
		if(rInst->playerIDs[i] != 0){
			return(0);
		}
	}

	return(1);
}