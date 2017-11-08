#ifndef socketServer_h
#define socketServer_h


#include <stdio.h>

#include "socketHandler.h"


typedef struct socketServer socketServer;
typedef struct socketServer {
	socketHandler connectionHandler;

	int type;
	int protocol;

    char *buffer;
    int bufferLength;
    size_t maxBufferSize;

	void (*buffFunc)(socketServer *server, const size_t clientID);
	void (*discFunc)(socketServer *server, const size_t clientID);
} socketServer;


unsigned char serverSetup();
void serverCleanup();

unsigned char serverInit(socketServer *server, const int type, const int protocol, char *ip, size_t ipLength, unsigned short port, size_t bufferSize,
                         void (*buffFunc)(socketServer *server, const size_t clientID), void (*discFunc)(socketServer *server, const size_t clientID));

void serverError(const char *func, const int code);


#endif