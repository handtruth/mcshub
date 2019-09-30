#include "client.hpp"

#include <cstring>
#include <cassert>
#include <stdexcept>
#include <system_error>

#include <ekutils/log.hpp>

#include "hosts_db.hpp"
#include "resources.hpp"

namespace mcshub {

template <typename P>
bool gate::paket_read(P & packet) {
	std::int32_t id, size;
	if (!head(id, size))
		return false;
	int s = packet.read(input.data(), size);
	if (std::int32_t(s) != size)
		throw bad_request("packet format error " + std::to_string(s) + " " + std::to_string(size));
	input.move(size);
	return true;
}

template <typename P>
void gate::paket_write(const P & packet) {
	std::size_t size = 10 + packet.size();
	output.asize(size);
	int s = packet.write(output.data() + output.size() - size, size);
#	ifdef DEBUG
		if (s < 0)
			throw "IMPOSSIBLE SITUATION";
#	endif
	output.ssize(size - s);
	send(); // Init transmission
}

bool gate::head(std::int32_t & id, std::int32_t & size) const {
	int s = pakets::head(input.data(), input.size(), size, id);
	if (s == -1)
		return false;
	if (size < 0)
		throw bad_request("packet size is lower than 0");
	conf_snap conf;
	std::int32_t max_size = conf->max_packet_size;
	if (max_size != -1 && size > max_size)
		throw bad_request("the maximum allowed packet size was reached");
	size += s;
	return std::size_t(size) <= input.size();
}

void gate::kostilA() {
	pakets::request req;
	std::size_t sz = req.size() + 10;
	input.asize(sz);
	int r = req.write(input.data() + input.size() - sz, sz);
	assert(r != -1);
	input.ssize(sz - r);
}

void gate::kostilB(const std::string & nick) {
	pakets::login login;
	login.name() = nick;
	std::size_t size = login.size() + 10;
	input.asize(size);
	int r = login.write(input.data() + input.size() - size, size);
	assert(r != -1);
	input.ssize(size - r);
}

void gate::tunnel(gate & other) {
	other.output.append(input.data(), input.size());
	input.clear();
	other.send();
}

void gate::receive() {
	std::size_t avail = sock.avail();
	std::size_t old = input.size();
	input.asize(avail);
	sock.read(input.data() + old, avail);
}

void gate::send() {
	if (output.size() == 0)
		return;
	int written = sock.write(output.data(), output.size());
	if (written == -1)
		return;
	output.move(written);
}

std::atomic<long> portal::globl_id = 0;

void portal::set_from_state_by_hs() {
	switch (hs.state()) {
	case 1:
		from_s = state_t::status_fake;
		return;
	case 2:
		from_s = state_t::login_fake;
		return;
	default:
		throw bad_request("handshake has a bad state: " + std::to_string(hs.state()));
	}
}

const settings::basic_record & portal::record(const conf_snap & conf) {
	const auto & servers = conf->servers;
	server_name = hs.address().c_str();
	std::string & name = server_name;
	std::size_t name_sz = name.size();
	const std::string & domain = conf->domain;
	std::size_t domain_sz = domain.size();
	bool is_fml = name.size() != hs.address().size();
	if (name[name_sz - 1] == '.')
		name.pop_back();
	if (name_sz > domain_sz && !std::strcmp(name.c_str() + name_sz - domain_sz, domain.c_str())) {
		name.resize(name_sz - domain_sz);
	}
	f_vars.srv_name = server_name;
	i_vars.srv_name = server_name;
	const auto & iter = servers.find(name);
	if (iter != servers.end()) {
		const settings::server_record & r = iter->second;
		if (is_fml && r.fml)
			return *r.fml;
		return r;
	}
	const settings::server_record & r = conf->default_server;
	if (is_fml && r.fml)
		return *r.fml;
	return r;
}

std::string portal::resolve_status() {
	const auto & record = rec.get();
	srv_vars.vars = &record.vars;
	std::ifstream file(record.status);
	log_debug("open status file: " + record.status);
	if (!file) {
		if (!record.status.empty())
			log_warning("status file '" + record.status + "' not accessible");
		if (record.mcsman)
			return vars.resolve(res::config::mcsman::status_json);
		else
			return vars.resolve(res::config::fallback::status_json);
	}
	std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
	return vars.resolve(content);
}

std::string portal::resolve_login() {
	const auto & record = rec.get();
	srv_vars.vars = &record.vars;
	std::ifstream file(record.login);
	log_debug("open login file: " + record.login);
	if (!file) {
		if (!record.status.empty())
			log_warning("login file '" + record.login + "' not accessible");
		if (record.mcsman)
			return vars.resolve(res::config::mcsman::login_json);
		else
			return vars.resolve(res::config::fallback::login_json);
	}
	std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
	return vars.resolve(content);
}

void portal::process_from_request() {
	switch (from_s) {
		case state_t::handshake:
			return from_handshake();
		case state_t::wait:
			return;
		case state_t::login:
			return from_login();
		case state_t::status_fake:
			return from_fake_status();
		case state_t::login_fake:
			return from_fake_login();
		case state_t::proxy_stable:
		case state_t::proxy:
			return from_proxy();
		default:
			throw std::runtime_error("illegal state, it seems to be a bug here");
	}
}

void portal::from_handshake() {
	if (!from.paket_read(hs))
		return;
	log_debug("client #" + std::to_string(id) + " send handshake: " + std::to_string(hs));
	const auto & r = record(conf);
	if (r.drop)
		throw bad_request("drop");
	rec = r;
	if (!r.address.empty() && r.port) {
		try {
			connect(to.sock, r.address, r.port);
			to_s = state_t::connect;
			from_s = state_t::wait;
			log_verbose("attempt to connect to " + r.address + ":" + std::to_string(r.port));
			using namespace ekutils::actions;
			poll.add(to.sock, in | out | rdhup | err | et, [this](auto &, std::uint32_t events) {
				on_to_event(events);
			});
			timeout = poll.later(std::chrono::milliseconds { conf->timeout }, [this]() {
				on_timeout();
			});
			return;
		} catch (const ekutils::dns_error &) {

		}
	}
	set_from_state_by_hs();
	process_from_request(); // Вообще это костыль
}

void portal::from_login() {
	pakets::login login;
	if (!from.paket_read(login))
		return;
	log_info("player \"" + login.name() + "\" connected to server \"" +
		hs.address() + "\" with connection id #" + std::to_string(id));
	from_s = state_t::proxy;
	to.paket_write(login);
	nickname = std::move(login.name());
}

void portal::from_fake_status() {
	std::int32_t id, size;
	if (!from.head(id, size))
		return;
	switch (id) {
		case pakets::ids::request: {
			pakets::request req;
			if (!from.paket_read(req))
				return;
			log_verbose("status request from connection #" + std::to_string(id));
			pakets::response response;
			response.message() = resolve_status();
			from.paket_write(response);
			break;
		}
		case pakets::ids::pingpong: {
			pakets::pinpong ping_pong;
			if (!from.paket_read(ping_pong))
				return;
			log_verbose("ping request from connection #" + std::to_string(id));
			from.paket_write(ping_pong);
			break;
		}
		default:
			throw bad_request("unexpected packet ID");
	}
}

void portal::from_fake_login() {
	pakets::login login;
	if (!from.paket_read(login))
		return;
	log_info("player \"" + login.name() + "\" tried to connect to server \"" +
		hs.address() + "\" with connection id #" + std::to_string(id));
	pakets::disconnect dc;
	dc.message() = resolve_login();
	from.paket_write(dc);
	disconnect();
}

void portal::from_proxy() {
	from.tunnel(to);
}

void portal::process_to_request() {
	switch (to_s) {
		case state_t::proxy:
		case state_t::proxy_stable:
			return to_proxy();
		case state_t::connect:
			return;
		default:
			throw std::runtime_error("illegal state, it seems to be a bug here");
	}
}

void portal::to_send_new_hs() {
	pakets::handshake new_hs = hs;
	to.paket_write(new_hs);
	assert(poll.refuse(timeout));
	to_s = state_t::proxy;
	from_s = (hs.state() == 1) ? state_t::proxy : state_t::login;
	process_to_request();
	process_from_request();
}

void portal::to_proxy() {
	to.tunnel(from);
}

portal::portal(ekutils::tcp_socket_d && sock, ekutils::epoll_d & p) :
	id(globl_id++), from(std::move(sock)), poll(p), rec(std::ref(conf->default_server)),
	vars(main_vars, srv_vars, f_vars, hs, env_vars) {}

void portal::on_from_event(std::uint32_t events) {
	using namespace ekutils;
	try {
		if (events & actions::rdhup) {
			// Just disconnect event from client
			disconnect();
		}
		if (events & actions::err) {
			// Disconnect with async error
			int err = errno;
			log_debug("async connection error received from client #" + std::to_string(id));
			throw std::system_error(std::make_error_code(std::errc(err)), "async error");
		}
		if (events & actions::in) {
			// New data for work has received
			from.receive();
			//std::int32_t id, sz;
			//while (from.head(id, sz))
			process_from_request();
		}
		if (events & actions::out) {
			// Out information is ready to be send
			from.send();
		}
	} catch (const std::exception & e) {
		log_error("connection error on client #" + std::to_string(id));
		log_error(e);
		disconnect();
	}
}

void portal::on_to_event(std::uint32_t events) {
	try {
		using namespace ekutils;
		if (events & actions::rdhup) {
			// Just disconnect event from backend server
			disconnect();
		}
		if (events & actions::hup) {
			// One case
			switch (to_s) {
				case state_t::connect: {
					// Close server gate and send fake status instead
					set_from_state_by_hs();
					poll.remove(to.sock);
					return;
				}
				default: {
					// Disconnect
					disconnect();
				}
			}
		}
		if (events & actions::err) {
			switch (to_s) {
				case state_t::connect:
				case state_t::proxy: {
					// Close server gate and send fake status instead
					set_from_state_by_hs();
					log_debug("error occured while backend " + std::string(to.sock.remote_endpoint())
						+ " connect process: " + std::make_error_code(std::errc(errno)).message());
					if (from.avail_read() < 2) {
						if (hs.state() == 1)
							from.kostilA();
						else
							from.kostilB(nickname);
					}
					process_from_request();
					to.sock.close();
					return;
				}
				default: {
					// Disconnect with async error
					int err = errno;
					log_debug("async connection error received from client #" + std::to_string(id) + ", state #" + std::to_string(int(to_s)));
					throw std::system_error(std::make_error_code(std::errc(err)), "async error");
				}
			}
		}
		if (events & actions::in) {
			// New data for work has received
			to.receive();
			process_to_request();
		}
		if (events & actions::out) {
			switch (to_s) {
				case state_t::connect: {
					std::errc err = to.sock.last_error();
					if (err == std::errc(0))
						err = to.sock.last_error();
					if (err == std::errc(0)) {
						// Async connection established
						log_verbose("async connection #" + std::to_string(id) + " established");
						to_send_new_hs();
					} else {
						log_verbose("async connection #" + std::to_string(id) + " to backend server failed");
						set_from_state_by_hs();
						to.sock.close(); // This step will destroy current lambda object, not safe
						return;
					}
					break;
				}
				default:
					// Out information is ready to be send
					to.send();
					to_s = state_t::proxy_stable;
					from_s = state_t::proxy_stable;
					process_from_request();
					break;
			}
		}
	} catch (const std::exception & e) {
		if (to_s == state_t::connect || to_s == state_t::proxy) {
			std::errc err = to.sock.ensure_connected();
			if (err == std::errc(0))
				err = to.sock.last_error();
			if (err == std::errc(0)) {
				// Async connection established
				log_verbose("async connection #" + std::to_string(id) + " established");
				to_send_new_hs();
				return;
			} else {
				log_verbose("async connection #" + std::to_string(id) + " to backend server failed");
				set_from_state_by_hs();
				if (from.avail_read() < 2) {
					if (hs.state() == 1)
						from.kostilA();
					else
						from.kostilB(nickname);
				}
				to.sock.close(); // This step will destroy current lambda object, not safe
				return;
			}
		}
		log_error("connection error on backend server \"" + hs.address() + '\"');
		log_error(e);
		disconnect();
	}
}

void portal::on_disconnect() {
	try {
		// Not nessesary
		pakets::disconnect dc;
		dc.message() = resolve_login();
		from.paket_write(dc);
	} catch (...) {}
}

void portal::on_timeout() {
	switch (to_s) {
		case state_t::connect:
			set_from_state_by_hs();
			to.sock.close();
			log_debug("connection timeout #" + std::to_string(id));
			process_from_request();
			return;
		default:
			return;
	}
}

} // namespace mcshub
