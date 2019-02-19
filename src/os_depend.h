
#ifndef _OS_DEPEND_HEAD
#	define _OS_DEPEND_HEAD
#	ifdef _WIN32
		// Windows section
#		ifndef WIN32_LEAN_AND_MEAN
#			define WIN32_LEAN_AND_MEAN
#		endif
#		include <windows.h>
#		include <winsock2.h>
#		include <ws2tcpip.h>
		namespace mcshub {
			typedef HANDLE handle_t;
		}
#	else
		// Linux section
#		include <sys/socket.h>
#		include <netinet/in.h>
#		include <arpa/inet.h>
#		include <unistd.h>
#		include <sys/epoll.h>
#		include <netdb.h>
		namespace mcshub {
			typedef int handle_t;
		}
#	endif
#endif // !_OS_DEPEND_HEAD
