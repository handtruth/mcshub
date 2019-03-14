#ifndef _SERVER_HEAD
#define _SERVER_HEAD

#include "socket_d.h"
#include "reallobuf.h"
#include "primitives.h"
#include "mc_pakets.h"

namespace mcshub {

class client final {
private:
	pakets::handshake hs;
	tcp_socket_d sock;
	reallobuf buffer;
	reallobuf outbuff;
	size_t rem;
	enum class state_enum {
		handshake, status, login, proxy
	} state;
	void receive();
	int process_request(size_t avail);
	int handshake(size_t avail);
	int status(size_t avail);
	int login(size_t avail);
	int proxy(size_t avail);
public:
	client(tcp_socket_d && so);
	tcp_socket_d & socket() noexcept {
		return sock;
	}
	bool operator()(descriptor & fd, std::uint32_t events);
	~client();

};

}

#endif // _SERVER_HEAD
