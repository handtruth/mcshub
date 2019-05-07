#ifndef _SERVER_HEAD
#define _SERVER_HEAD

#include <memory>
#include <atomic>

#include "socket_d.h"
#include "reallobuf.h"
#include "primitives.h"
#include "mc_pakets.h"
#include "response_props.h"
#include "settings.h"

namespace mcshub {

class client final {
private:
	pakets::handshake hs;
	tcp_socket_d sock;
	event & poll;
	tcp_socket_d proxy_sock;
	reallobuf buffer;
	reallobuf outbuff;
	size_t rem;
	server_vars srv_vars;
	file_vars f_vars;
	std::atomic_bool bs;
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
	void proxy_backend(std::uint32_t events);
	std::string resolve_status();
	const config::server_record & record(const std::shared_ptr<const config> & conf);
public:
	client(tcp_socket_d && so, event & epoll);
	tcp_socket_d & socket() noexcept {
		return sock;
	}
	bool is_busy() volatile const noexcept {
		return bs;
	}
	bool operator()(std::uint32_t events);
	~client();

};

}

#endif // _SERVER_HEAD
