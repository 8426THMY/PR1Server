#include <stdio.h>

#include "server/socketShared.h"
#include "server/socketServer.h"
#include "server/socketTCP.h"

#include "game/pr1_game.h"


int main(int argc, char *argv[]){
	if(serverSetup()){
		socketServer gameServer;

		if(gameLoadServer(&gameServer, SOCK_STREAM, IPPROTO_TCP)){
			gameInit();

			char running = 1;
			while(running){
				running = serverListenTCP(&gameServer);
			}

			serverCloseTCP(&gameServer);
			gameClose();
		}
    }
    serverCleanup();


	puts("\n\nPress enter to exit.");
	getc(stdin);


    return(1);
}