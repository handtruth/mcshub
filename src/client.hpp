#ifndef _CLIENT_HEAD
#define _CLIENT_HEAD

#include <future>
#include <atomic>
#include <cassert>

#include <ekutils/expandbuff.hpp>
#include <ekutils/socket_d.hpp>
#include <ekutils/primitives.hpp>
#include <ekutils/epoll_d.hpp>

#include "settings.hpp"
#include "mc_pakets.hpp"
#include "response_props.hpp"

namespace mcshub {

using ekutils::byte_t;

class bad_request : public std::runtime_error {
public:
	explicit bad_request(const std::string & message) : std::runtime_error(message) {}
	explicit bad_request(const char * message) : std::runtime_error(message) {}
};

class gate {
public:
private:
	ekutils::expandbuff input, output;
public:
	ekutils::tcp_socket_d sock;
	gate() {}
	explicit gate(ekutils::tcp_socket_d && socket) : sock(std::move(socket)) {}
	bool head(std::int32_t & id, std::int32_t & size) const;
	template <typename P>
	bool paket_read(P & packet);
	template <typename P>
	void paket_write(const P & packet);
	void kostilA();
	void kostilB(const std::string & nick);
	void tunnel(gate & other);
	void receive();
	void send();
	std::size_t avail_read() const noexcept {
		return input.size();
	}
	std::size_t avail_write() const noexcept {
		return output.size();
	}
};

class portal {
	static std::atomic<long> globl_id;
	std::string nickname;
	long id;
	int timeout;
	gate from, to;
	ekutils::epoll_d & poll;
	pakets::handshake hs;
	std::string server_name;
	server_vars srv_vars;
	file_vars f_vars;
	img_vars i_vars;
	conf_snap conf;
	std::reference_wrapper<const settings::basic_record> rec;
	vars_manager<main_vars_t, server_vars, file_vars, img_vars, pakets::handshake, env_vars_t> vars;
	enum class state_t {
		handshake, connect, wait, status_fake, login_fake, login, proxy, proxy_stable, ping
	} from_s = state_t::handshake, to_s;
	bool disconnected = false;
	void disconnect() noexcept {
		disconnected = true;
	}
	void set_from_state_by_hs();
	const settings::basic_record & record(const conf_snap & conf);
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
	portal(ekutils::tcp_socket_d && sock, ekutils::epoll_d & p);
	bool is_disconnected() const noexcept {
		return disconnected;
	}
	ekutils::tcp_socket_d & sock() {
		return from.sock;
	}
	void on_from_event(std::uint32_t events);
	void on_to_event(std::uint32_t events);
	void on_disconnect();
private:
	void on_timeout();
};

} // namespace mcshub

#endif
