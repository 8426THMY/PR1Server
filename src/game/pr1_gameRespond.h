#ifndef pr1_gameRespond_h
#define pr1_gameRespond_h


#include "../server/socketServer.h"

#include "pr1_player.h"


void gameRespondLogin(socketServer *server, const size_t clientID, player *sender);
void gameRespondEnterLobby(const socketServer *server, player *sender);
void gameRespondSendChatMessage(const socketServer *server, const player *sender);
void gameRespondJoinRace(const socketServer *server, player *sender);
void gameRespondReadyUp(const socketServer *server, const player *sender);
void gameRespondRankUp(const socketServer *server, player *sender);
void gameRespondPlayRace(const socketServer *server, const player *sender);
void gameRespondStartRace(const socketServer *server, size_t raceNum);
void gameRespondLeaveRace(const socketServer *server, player *sender);
void gameRespondFinishRace(const socketServer *server, const player *sender);
void gameRespondDomainPolicy(const socketServer *server, const size_t clientID);


#endif