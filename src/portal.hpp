#ifndef CLIENT_HEAD_QCXPBLLROBOJJWKJ
#define CLIENT_HEAD_QCXPBLLROBOJJWKJ

#include <future>
#include <atomic>
#include <cassert>

#include <ekutils/tcp_d.hpp>
#include <ekutils/primitives.hpp>
#include <ekutils/epoll_d.hpp>
#include <ekutils/idgen.hpp>

#include "gate.hpp"
#include "settings.hpp"
#include "mc_pakets.hpp"
#include "response_props.hpp"

namespace mcshub {

class portal {
	static ekutils::idgen<long> globl_id;
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
	portal(stream_sock && sock, ekutils::epoll_d & p);
	bool is_disconnected() const noexcept {
		return disconnected;
	}
	ekutils::net::stream_socket_d & sock() {
		return *from.sock;
	}
	void on_from_event(std::uint32_t events);
	void on_to_event(std::uint32_t events);
	void on_disconnect();
private:
	void on_timeout();
};

} // namespace mcshub

#endif // CLIENT_HEAD_QCXPBLLROBOJJWKJ
