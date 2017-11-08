#ifndef socketTCP_h
#define socketTCP_h


#include <stdio.h>

#include "socketServer.h"


unsigned char serverListenTCP(socketServer *server);
unsigned char serverSendTCP(const socketServer *server, const size_t clientID, const char *buffer, const size_t bufferLength);
void serverDisconnectTCP(socketServer *server, const size_t clientID);
void serverCloseTCP(socketServer *server);


#endif