#include "socketShared.h"


#ifdef SERVER_USE_POLL
	#ifdef _WIN32
		int WSAAPI WSAPoll(struct pollfd *fdarray, ULONG nfds, int timeout);
		int pollFunc(struct pollfd *fdarray, size_t nfds, int timeout){
			return(WSAPoll(fdarray, nfds, timeout));
		}
	#else
		int pollFunc(struct pollfd *fdarray, size_t nfds, int timeout){
			return(poll(fdarray, nfds, timeout));
		}
	#endif
#else
	//This function is slower than it has to be... so don't use XP!
	int pollFunc(struct pollfd *fdarray, size_t nfds, int timeout){
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
	}
#endif