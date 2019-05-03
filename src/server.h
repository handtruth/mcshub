#ifndef _SERVER_HEAD
#define _SERVER_HEAD

#include "socket_d.h"
#include "reallobuf.h"
#include "primitives.h"
#include "mc_pakets.h"
#include "response_props.h"

#include <memory>

namespace mcshub {

class client final {
private:
	pakets::handshake hs;
	tcp_socket_d sock;
	event & poll;
	std::unique_ptr<tcp_socket_d> proxy_sock;
	reallobuf buffer;
	reallobuf outbuff;
	size_t rem;
	server_vars srv_vars;
	file_vars f_vars;
	vars_manager<main_vars_t, server_vars, file_vars, pakets::handshake, env_vars_t> vars;
	enum class state_enum {
		handshake, status, login, proxy
	} state;
	void receive();
	int process_request(size_t avail);
	int handshake(size_t avail);
	int status(size_t avail);
	int login(size_t avail);
	int proxy(size_t avail);
	/////
	std::string resolve_status();
public:
	client(tcp_socket_d && so, event & epoll);
	tcp_socket_d & socket() noexcept {
		return sock;
	}
	bool operator()(std::uint32_t events);
	~client();

};

}

#endif // _SERVER_HEAD
