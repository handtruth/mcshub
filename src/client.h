#ifndef _CLIENT_HEAD
#define _CLIENT_HEAD

#include <future>
#include <atomic>

#include "reallobuf.h"
#include "socket_d.h"
#include "primitives.h"
#include "event_pull.h"
#include "settings.h"
#include "mc_pakets.h"
#include "response_props.h"
#include "timer_d.h"

namespace mcshub {

class bad_request : public std::runtime_error {
public:
	bad_request(const std::string & message) : std::runtime_error(message) {}
	bad_request(const char * message) : std::runtime_error(message) {}
};

class gate {
public:
	struct data_flow {
		reallobuf buff;
		size_t sz = 0;
	};
private:
	data_flow input, output;
public:
	tcp_socket_d sock;
	gate() {}
	gate(tcp_socket_d && socket) : sock(std::move(socket)) {}
	bool head(std::int32_t & id, std::int32_t & size) const;
	template <typename P>
	bool paket_read(P & packet);
	template <typename P>
	void paket_write(const P & packet);
	void kostilA() {
		pakets::request req;
		input.sz += req.write(input.buff.data() + input.sz, 2);
	}
	void kostilB(const std::string & nick) {
		pakets::login login;
		login.name() = nick;
		size_t size = login.size();
		input.buff.probe(input.sz + size + 10);
		input.sz += login.write(input.buff.data() + input.sz, size + 10);
	}
	void tunnel(gate & other);
	void read_async(byte_t bytes[], size_t size);
	void write_async(const byte_t bytes[], size_t size);
	void receive();
	void send();
	size_t avail_read() const noexcept {
		return input.sz;
	}
	size_t avail_write() const noexcept {
		return output.sz;
	}
};

class portal {
	static std::atomic<long> globl_id;
	timer_d timeout;
	std::string nickname;
	long id;
	gate from, to;
	event & poll;
	pakets::handshake hs;
	server_vars srv_vars;
	file_vars f_vars;
	conf_snap conf;
	std::reference_wrapper<const config::server_record> rec;
	vars_manager<main_vars_t, server_vars, file_vars, pakets::handshake, env_vars_t> vars;
	enum class state_t {
		handshake, connect, wait, status_fake, login_fake, login, proxy, proxy_stable, ping
	} from_s = state_t::handshake, to_s;
	bool disconnected = false;
	void disconnect() noexcept {
		disconnected = true;
	}
	void set_from_state_by_hs();
	const config::server_record & record(const conf_snap & conf);
	std::string resolve_status();
	std::string resolve_login();
	void process_from_request();
	void from_handshake();
	void from_login();
	void from_fake_status();
	void from_fake_login();
	void from_proxy();
	void process_to_request();
	void to_send_new_hs();
	void to_proxy();
public:
	portal(tcp_socket_d && sock, event & p);
	bool is_disconnected() const noexcept {
		return disconnected;
	}
	tcp_socket_d & sock() {
		return from.sock;
	}
	void on_from_event(std::uint32_t events);
	void on_to_event(std::uint32_t events);
	void on_disconnect();
private:
	void on_timeout(std::uint32_t events);
};

template <typename P>
bool gate::paket_read(P & packet) {
	std::int32_t id, size;
	if (!head(id, size))
		return false;
	int s = packet.read(input.buff.data(), size);
	if (std::int32_t(s) != size)
		throw bad_request("packet format error " + std::to_string(s) + " " + std::to_string(size));
	input.sz -= size;
	input.buff.move(size);
	return true;
}

template <typename P>
void gate::paket_write(const P & packet) {
	size_t size = 10 + packet.size();
	output.buff.probe(output.sz + size);
	int s = packet.write(output.buff.data() + output.sz, size);
#	ifdef DEBUG
		if (s < 0)
			throw "IMPOSSIBLE SITUATION";
#	endif
	output.sz += s;
	send(); // Init transmission
}

} // namespace mcshub

#endif
