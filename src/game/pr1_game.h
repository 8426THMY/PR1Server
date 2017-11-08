#ifndef pr1_game_h
#define pr1_game_h


#include "../server/socketServer.h"


void gameInit();
unsigned char gameLoadServer(socketServer *server, const int type, const int protocol);
void gameClose();


#endif