#include "client.h"

#include <cstring>
#include <stdexcept>
#include <system_error>

#include "log.h"
#include "hosts_db.h"
#include "resources.h"

namespace mcshub {

bool gate::head(std::int32_t & id, std::int32_t & size) const {
	int s = pakets::head(input.buff.data(), input.sz, size, id);
	if (s == -1)
		return false;
	if (size < 0)
		throw bad_request("packet size is lower than 0");
	conf_snap conf;
	std::int32_t max_size = conf->max_packet_size;
	if (max_size != -1 && size > max_size)
		throw bad_request("the maximum allowed packet size was reached");
	size += s;
	return size_t(size) <= input.sz;
}

void gate::tunnel(gate & other) {
	size_t new_sz = input.sz + other.output.sz;
	other.output.buff.probe(new_sz);
	std::memcpy(other.output.buff.data() + other.output.sz, input.buff.data(), input.sz);
	other.output.sz = new_sz;
	input.sz = 0;
	other.send();
}

void gate::read_async(byte_t bytes[], size_t size) {
	if (size > input.sz) {
		return;
	}
	std::memcpy(bytes, input.buff.data(), size);
	input.buff.move(size);
	input.sz -= size;
}

void gate::write_async(const byte_t bytes[], size_t size) {
	output.buff.probe(size + output.sz);
	std::memcpy(output.buff.data() + output.sz, bytes, size);
	size_t new_sz;
	new_sz = output.sz += size;
	int written = sock.write(output.buff.data(), new_sz); // required for new EPOLLOUT event
	output.buff.move(written);
	output.sz -= written;
}

void gate::receive() {
	size_t avail = sock.avail();
	input.buff.probe(input.sz + avail);
	sock.read(input.buff.data() + input.sz, avail);
	input.sz += avail;
}

void gate::send() {
	if (output.sz == 0)
		return;
	int written = sock.write(output.buff.data(), output.sz);
	if (written == -1)
		return;
	output.buff.move(written);
	output.sz -= written;
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

const config::server_record & portal::record(const conf_snap & conf) {
	const auto & servers = conf->servers;
	std::string name = hs.address().c_str();
	std::size_t name_sz = name.size();
	const std::string & domain = conf->domain;
	std::size_t domain_sz = domain.size();
	bool allowed = name.size() == hs.address().size();
	if (name[name_sz - 1] == '.')
		name.pop_back();
	if (name_sz > domain_sz && !std::strcmp(name.c_str() + name_sz - domain_sz, domain.c_str())) {
		name.resize(name_sz - domain_sz);
	}
	const auto & iter = servers.find(name);
	if (iter != servers.end()) {
		const config::server_record & r = iter->second;
		if (allowed || r.allowFML)
			return r;
	}
	const config::server_record & r = conf->default_server;
	if (allowed || r.allowFML)
		return r;
	throw bad_request("modded client is forbidden");
}

std::string portal::resolve_status() {
	const auto & record = rec.get();
	srv_vars.vars = &record.vars;
	std::ifstream file(record.status);
	log_debug("open status file: " + record.status);
	if (!file) {
		log_warning("status file '" + record.status + "' not accessible");
		if (record.mcsman)
			return vars.resolve(res::sample_mcsman_status_json, res::sample_mcsman_status_json_len);
		else
			return vars.resolve(res::sample_default_status_json, res::sample_default_status_json_len);
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
		log_warning("login file '" + record.login + "' not accessible");
		if (record.mcsman)
			return vars.resolve(res::sample_mcsman_login_json, res::sample_mcsman_login_json_len);
		else
			return vars.resolve(res::sample_default_login_json, res::sample_default_login_json_len);
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
	rec = r;
	if (!r.address.empty() && r.port) {
		try {
			connect(to.sock, r.address, r.port);
			to_s = state_t::connect;
			from_s = state_t::wait;
			log_verbose("attempt to connect to " + r.address + ":" + std::to_string(r.port));
			using namespace actions;
			poll.add(to.sock, in | out | rdhup | err | et, [this](descriptor &, std::uint32_t events) {
				on_to_event(events);
			});
			poll.add(timeout, [this](descriptor &, std::uint32_t events) {
				on_timeout(events);
			});
			timeout.delay(std::chrono::milliseconds { conf->timeout });
			return;
		} catch (const dns_error &) {

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
	const auto & r = rec.get();
	new_hs.address() = r.address;
	new_hs.port() = r.port;
	to.paket_write(new_hs);
	to_s = state_t::proxy;
	from_s = (hs.state() == 1) ? state_t::proxy : state_t::login;
	process_to_request();
	process_from_request();
}

void portal::to_proxy() {
	to.tunnel(from);
}

portal::portal(tcp_socket_d && sock, event & p) :
	id(globl_id++), from(std::move(sock)), poll(p), rec(std::ref(conf->default_server)),
	vars(main_vars, srv_vars, f_vars, hs, env_vars) {}

void portal::on_from_event(std::uint32_t events) {
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

void portal::on_timeout(std::uint32_t events) {
	if (events & actions::in) {
		switch (to_s) {
			case state_t::connect:
				timeout.read();
				set_from_state_by_hs();
				to.sock.close();
				log_debug("connection timeout #" + std::to_string(id));
				process_from_request();
				return;
			default:
				return;
		}
	}
}

} // namespace mcshub
