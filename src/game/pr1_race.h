#ifndef pr1_race_h
#define pr1_race_h


#define RACE_NUM_MAPS  8
#define RACE_NUM_SLOTS 4


#include <string.h>

#include "../vector.h"


//For each race, store the I.D. and state of the clients in each slot.
//We store one of these for each of the 8 races in the game.
typedef struct race {
	size_t slotID[RACE_NUM_SLOTS];
	unsigned char slotState[RACE_NUM_SLOTS];
} race;

typedef struct raceInstance {
	char map;

	size_t playerIDs[RACE_NUM_SLOTS];
	size_t totalPlayers;
	size_t playersFinished;
} raceInstance;


void raceInit(race *r);
void raceInstInit(raceInstance *rInst);

unsigned char raceJoinSlot(race *r, const size_t slot, const size_t clientID);
unsigned char raceLeaveSlot(race *r, const size_t slot, const size_t clientID);
unsigned char raceReadyUp(race *r, const size_t slot, const size_t clientID);

unsigned char raceInstAddPlayer(raceInstance *rInst, const size_t slot, const size_t clientID);
unsigned char raceInstRemovePlayer(raceInstance *rInst, const size_t slot, const size_t clientID);
float raceInstCalculateRank(const raceInstance *rInst, const float oldRank);

unsigned char raceIsReady(const race *r);
unsigned char raceInstEmpty(raceInstance *rInst);


extern race raceList[RACE_NUM_MAPS];
extern vector currentRaces;


#endif