#include "socketServer.h"


#include <stdlib.h>
#include <string.h>

#include "socketShared.h"


unsigned char serverSetup(){
	#ifdef _WIN32
	//Initialize the Winsock DLL.
	WSADATA wsaData;
	int wsaError = WSAStartup(WS_VERSION, &wsaData);
	if(wsaError != 0){
		serverError("WSAStartup()", lastErrorID);

		return(0);
	}
	#endif

	return(1);
}

void serverCleanup(){
	#ifdef _WIN32
	WSACleanup();
	#endif
}


unsigned char serverInit(socketServer *server, const int type, const int protocol, char *ip, size_t ipLength, unsigned short port, size_t bufferSize,
                         void (*buffFunc)(socketServer *server, const size_t clientID), void (*discFunc)(socketServer *server, const size_t clientID)){

	//Set the server details!
	char tempIP[46];
	if(ip != NULL && ipLength < 46){
		memcpy(tempIP, ip, ipLength);
		tempIP[ipLength] = '\0';
	}else{
		tempIP[0] = '\0';
	}

	server->type = type;
	server->protocol = protocol;

	server->maxBufferSize = bufferSize;


	//Check the address family that the specified I.P. is using.
	char tempBuffer[16];
	int af;
	if(inet_pton(AF_INET, tempIP, tempBuffer)){
		af = AF_INET;
	}else if(inet_pton(AF_INET6, tempIP, tempBuffer)){
		af = AF_INET6;
	}else{
		af = DEFAULT_ADDRESS_FAMILY;
	}


	//Create a TCP/UDP socket that can be bound to IPv4 or IPv6 addresses.
	struct pollfd masterFD;
	masterFD.fd = socket(af, type, protocol);
	if(masterFD.fd == INVALID_SOCKET){
		serverError("socket()", lastErrorID);

		return(0);
	}
	masterFD.events = POLLIN;
	masterFD.revents = 0;


	//If SERVER_POLL_TIMEOUT is greater than 0, set the timeout for recvfrom.
	if(SERVER_POLL_TIMEOUT >= 0){
		struct timeval timeoutValue;

		if(SERVER_POLL_TIMEOUT == 0){
			//Make sure our polling function is non-blocking.
			unsigned long argp = 1;
			ioctl(masterFD.fd, FIONBIO, &argp);

			timeoutValue.tv_sec = 0;
			timeoutValue.tv_usec = 0;
		}else{
			timeoutValue.tv_sec = SERVER_POLL_TIMEOUT / 1000;
			timeoutValue.tv_usec = (SERVER_POLL_TIMEOUT - timeoutValue.tv_sec * 1000) * 1000;
		}

		//Set the timeout for recvfrom!
		if(setsockopt(masterFD.fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeoutValue, sizeof(timeoutValue)) != 0){
			serverError("setsockopt()", lastErrorID);

			return(0);
		}
	}


	socketInfo masterInfo;
	memset(&masterInfo.addr, 0, sizeof(struct sockaddr_storage));
	if(af == AF_INET){
		//Fill up the address!
		if(!inet_pton(af, tempIP, (char *)&(((struct sockaddr_in *)&masterInfo.addr)->sin_addr))){
			((struct sockaddr_in *)&masterInfo.addr)->sin_addr.s_addr = INADDR_ANY;
			strcpy(tempIP, "INADDR_ANY");
		}

		((struct sockaddr_in *)&masterInfo.addr)->sin_family = af;
		((struct sockaddr_in *)&masterInfo.addr)->sin_port = htons(port);

		masterInfo.addrSize = sizeof(struct sockaddr_in);
	}else if(af == AF_INET6){
		//Create a variable for our IPv6 socket info and fill it up!
		if(!inet_pton(af, tempIP, (char *)&(((struct sockaddr_in6 *)&masterInfo.addr)->sin6_addr))){
			((struct sockaddr_in6 *)&masterInfo.addr)->sin6_addr = in6addr_any;
			strcpy(tempIP, "in6addr_any");
		}

		((struct sockaddr_in6 *)&masterInfo.addr)->sin6_family = af;
		((struct sockaddr_in6 *)&masterInfo.addr)->sin6_port = htons(port);

		masterInfo.addrSize = sizeof(struct sockaddr_in6);
	}
	//Bind our socket to the address!
	if(bind(masterFD.fd, (struct sockaddr *)&masterInfo.addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR){
		serverError("bind()", lastErrorID);

		return(0);
	}

	//Start listening for connections if the socket is using TCP!
	if(protocol == IPPROTO_TCP){
		if(listen(masterFD.fd, SOMAXCONN) == SOCKET_ERROR){
			serverError("listen()", lastErrorID);

			return(0);
		}
	}

	//Initialize our buffer for received data!
	server->buffer = malloc((server->maxBufferSize + 1) * sizeof(*server->buffer));
	if(server->buffer == NULL){
		puts("Error: Could not allocate sufficient memory for the receive buffer.\n");

		return(0);
	}
	server->bufferLength = 0;
	server->buffer[server->bufferLength] = '\0';


	//Initialize our connectionHandler and push the master socket into it!
	if(!handlerInit(&server->connectionHandler, SERVER_MAX_SOCKETS, &masterFD, &masterInfo)){
		puts("Error: Could not create a handle for the master socket.\n");

		return(0);
	}


	server->buffFunc = buffFunc;
	server->discFunc = discFunc;


	printf("Server was initialized successfully!\n\n"
		   "Listening on %s:%u...\n\n\n", tempIP, port);


	return(1);
}


//Print a socket-related error code!
void serverError(const char *func, const int code){
	printf("There was a problem with socket function %s!\n"
	       "Error: %d\n"
	       "Please check the link below for more information:\n"
	       "https://msdn.microsoft.com/en-us/library/windows/desktop/ms740668\n",
	       func, code);
}