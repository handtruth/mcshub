#ifndef _SERVER_HEAD
#define _SERVER_HEAD

#include <memory>
#include <atomic>
#include <list>

#include "socket_d.h"
#include "reallobuf.h"
#include "primitives.h"
#include "mc_pakets.h"
#include "response_props.h"
#include "settings.h"

namespace mcshub {

struct dbuff final {
	reallobuf buff;
	size_t avail;
	dbuff() {}
	int send_to(tcp_socket_d & other);
	operator byte_t * () noexcept {
		return buff.data();
	}
	size_t probe(size_t size) {
		return buff.probe(size);
	}
};

enum class state_enum {
	/* client|server */ handshake,
	/* client */        ask,
	/* client */        status,
	/* client|server */ login,
	/* client|server */ proxy
};

struct ts_state final {
	state_enum state;
	tcp_socket_d sock;
	dbuff buff;
	void receive();
	int send_to(ts_state & other) {
		return buff.send_to(other.sock);
	}
};

struct worker;

class client final {
private:
	pakets::handshake hs;
	server_vars srv_vars;
	file_vars f_vars;

	worker & father;
	std::list<client>::iterator me;

	ts_state client_s, server_s;
	dbuff extra_buff;

	vars_manager<main_vars_t, server_vars, file_vars, pakets::handshake, env_vars_t> vars;

	void receive_client();
	///// CLIENT
	int process_request(size_t avail);
	int handshake(size_t avail);
	int status(size_t avail);
	int login(size_t avail);
	int proxy(size_t avail);
	/////
	void proxy_backend(std::uint32_t events);
	std::string resolve_status();
	const config::server_record & record(const std::shared_ptr<const config> & conf);
	void disconnect();
	void set_state_by_hs();
public:
	client(tcp_socket_d && sock, worker & w);
	void set_me(const std::list<client>::iterator & itself) {
		me = itself;
	}
	tcp_socket_d & client_socket() noexcept {
		return client_s.sock;
	}
	void on_client_sock(descriptor &, std::uint32_t events);
	void on_server_sock(descriptor &, std::uint32_t events);
	void terminate();
	~client();

};

}

#endif // _SERVER_HEAD
