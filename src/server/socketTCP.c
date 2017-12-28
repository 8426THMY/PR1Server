#include "socketTCP.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


unsigned char serverListenTCP(socketServer *server){
	//Check if the any of the sockets have changed state.
	int changedSockets = pollFunc(server->connectionHandler.fds, server->connectionHandler.size, SERVER_POLL_TIMEOUT);
	if(changedSockets > 0){
		//If the master socket has returned with POLLIN, we're ready to accept a new connection!
		if(server->connectionHandler.fds[0].revents & POLLIN){
			//Store information pertaining to whoever sent the data!
			struct pollfd tempFD;
			socketInfo tempInfo;
			memset(&tempInfo.addr, 0, sizeof(tempInfo.addr));
			tempInfo.addrSize = sizeof(tempInfo.addr);

			tempFD.fd = accept(server->connectionHandler.fds[0].fd, (struct sockaddr *)&tempInfo.addr, &tempInfo.addrSize);

			//If the connection was accepted successfully, add it to the connectionHandler!
			if(tempFD.fd != INVALID_SOCKET){
				tempFD.events = POLLIN;
				tempFD.revents = 0;
				//tempInfo.lastUpdateTick = currentTick;

				handlerAdd(&server->connectionHandler, &tempFD, &tempInfo);
			}else{
				serverPrintError("accept()", serverGetLastError());
			}

			//Reset the master socket's return events!
			server->connectionHandler.fds[0].revents = 0;

			--changedSockets;
		}

		size_t i;
		//Now check if the other sockets have changed!
		for(i = 1; changedSockets > 0 && i < server->connectionHandler.size; ++i){
			//If the client has timed out, disconnect them!
			#warning "TCP timeout isn't implemented yet!"
			if(0){
				//

			//Otherwise, check if they've returned any events!
			}else if(server->connectionHandler.fds[i].revents != 0){
				//Store this stuff in case some clients disconnect!
				size_t oldID = server->connectionHandler.info[i].id;
				size_t oldSize = server->connectionHandler.size;

				//Check if the client has sent something!
				if(server->connectionHandler.fds[i].revents & POLLIN){
					//Check what they've sent!
					server->bufferLength = recv(server->connectionHandler.fds[i].fd, server->buffer, server->maxBufferSize, 0);
					server->buffer[server->bufferLength] = '\0';

					//The client has sent something, so handle it!
					if(server->bufferLength > 0){
						if(server->buffFunc != NULL){
							(*server->buffFunc)(server, server->connectionHandler.info[i].id);
						}

					//The client has disconnected, so disconnect them from our side!
					}else if(server->bufferLength == 0){
						serverDisconnectTCP(server, server->connectionHandler.info[i].id);

					//There was an error, so disconnect the client!
					}else{
						serverPrintError("recv()", serverGetLastError());
						serverDisconnectTCP(server, server->connectionHandler.info[i].id);
					}
				}

				//Check if we haven't disconnected the client!
				if(server->connectionHandler.idLinks[oldID] != 0){
					//If they've hung up, disconnect them from our end!
					if(server->connectionHandler.fds[i].revents & POLLHUP){
						serverDisconnectTCP(server, server->connectionHandler.info[i].id);

					//Otherwise, clear their return events!
					}else{
						server->connectionHandler.fds[i].revents = 0;
					}
				}
				//Move the iterator back if we have to!
				if(oldSize > server->connectionHandler.size){
					//If the current client didn't disconnect, just find their new index!
					if(server->connectionHandler.idLinks[oldID] != 0){
						i = server->connectionHandler.idLinks[oldID];

					//Otherwise, skip back using the difference in size. It's a bit inaccurate, but that's fine.
					}else{
						oldSize -= server->connectionHandler.size;
						if(i > oldSize){
							i -= oldSize;
						}else{
							i = 1;
						}
					}
				}

				--changedSockets;
			}
		}
	}else if(changedSockets == SOCKET_ERROR){
		serverPrintError(SERVER_POLL_FUNC, serverGetLastError());

		return(0);
	}


	return(1);
}

//Send a user a message!
unsigned char serverSendTCP(const socketServer *server, const size_t clientID, const char *buffer, const size_t bufferLength){
	if(clientID < server->connectionHandler.capacity && server->connectionHandler.idLinks[clientID] != 0){
		//If the client exists, send the buffer to them!
		if(send(server->connectionHandler.fds[server->connectionHandler.idLinks[clientID]].fd, buffer, bufferLength, 0) >= 0){
			return(1);
		}else{
			serverPrintError("send()", serverGetLastError());
		}
	}else{
		printf("Error: Tried to send data to an invalid socket.\n");
	}

	return(0);
}

//Disconnect a user!
void serverDisconnectTCP(socketServer *server, const size_t clientID){
	if(clientID < server->connectionHandler.capacity && server->connectionHandler.idLinks[clientID] != 0){
		if(server->discFunc != NULL){
			(*server->discFunc)(server, clientID);
		}

		socketclose(server->connectionHandler.fds[server->connectionHandler.idLinks[clientID]].fd);
		handlerRemove(&server->connectionHandler, clientID);
	}
}

//Shutdown the server!
void serverCloseTCP(socketServer *server){
	size_t i = server->connectionHandler.size;
	while(i > 1){
		--i;
		serverDisconnectTCP(server, server->connectionHandler.info[i].id);
	}
	socketclose(server->connectionHandler.fds[0].fd);
	handlerClear(&server->connectionHandler);

	free(server->buffer);
}