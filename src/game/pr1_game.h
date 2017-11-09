#ifndef pr1_game_h
#define pr1_game_h


#include "../server/socketServer.h"


unsigned char gameLoadServer(socketServer *server, const int type, const int protocol);

void gameInit();
void gameClose();


#endif