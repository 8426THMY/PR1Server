#include "socketShared.h"


//If mode is 0, make socket functions block. Otherwise, make them nonblocking.
unsigned char setNonBlockMode(const int fd, unsigned long mode){
	#ifdef _WIN32
		return(!ioctlsocket(fd, FIONBIO, &mode));
	#else
		int flags = fcntl(fd, F_GETFL, 0);
		if(flags < 0){
			return(0);
		}

		flags = mode ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
		return(!fcntl(fd, F_SETFL, flags));
	#endif
}

int pollFunc(struct pollfd *fdarray, size_t nfds, int timeout){
	#ifdef SERVER_USE_POLL
		#ifdef _WIN32
			return(WSAPoll(fdarray, nfds, timeout));
		#else
			return(poll(fdarray, nfds, timeout));
		#endif

	//This function is slower than it has to be... so don't use XP!
	#else
		fd_set changedSockets;
		int changedSocketsCount;
		const size_t totalSockets = nfds < SERVER_MAX_SOCKETS ? nfds : SERVER_MAX_SOCKETS;
		struct timeval timeoutValue;
		struct timeval *timeoutPointer = &timeoutValue;

		size_t a, b;
		//Add the socket descriptors to our fd_set!
		for(a = 0; a < totalSockets; ++a){
			changedSockets.fd_array[a] = fdarray[a].fd;
		}
		changedSockets.fd_count = totalSockets;


		//If timeout is greater than or equal to 0, the function shouldn't block.
		if(timeout > 0){
			timeoutValue.tv_sec = timeout / 1000;
			timeoutValue.tv_usec = (timeout - timeoutValue.tv_sec * 1000) * 1000;
		}else if(timeout == 0){
			timeoutValue.tv_sec = 0;
			timeoutValue.tv_usec = 0;

		//Otherwise, it should!
		}else{
			timeoutPointer = NULL;
		}


		//Check if the any of the sockets have changed state.
		changedSocketsCount = select(0, &changedSockets, NULL, NULL, timeoutPointer);
		if(changedSocketsCount != SOCKET_ERROR){
			//Update the return events for our pollfds!
			for(a = 0; a < changedSocketsCount; ++a){
				for(b = 0; b < totalSockets; ++b){
					if(changedSockets.fd_array[a] == fdarray[b].fd){
						fdarray[b].revents = POLLIN;

						break;
					}
				}
			}
		}


		return(changedSocketsCount);
	#endif
}


//Get the I.D. of the last error!
int serverGetLastError(){
	#ifdef _WIN32
		return(WSAGetLastError());
	#else
		const int lastErrorID = errno;
		errno = 0;
		return(lastErrorID);
	#endif
}

//Print a socket-related error code!
void serverPrintError(const char *func, const int code){
	printf("There was a problem with socket function %s!\n"
	       "Error: %d\n"
	       "Please check the link below for more information:\n"
	       "https://msdn.microsoft.com/en-us/library/windows/desktop/ms740668\n",
	       func, code);
}