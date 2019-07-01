#include "server.h"

#include <system_error>
#include <exception>
#include <stdexcept>
#include <cstring>

#include "log.h"
#include "resources.h"
#include "hosts_db.h"
#include "busy_state.h"
#include "putil.h"
#include "thread_controller.h"

namespace mcshub {

[[noreturn]]
void throw_sock_error() {
	int err = errno;
	throw std::system_error(std::make_error_code((std::errc)err));
}

int dbuff::send_to(tcp_socket_d & other) {
	if (!avail)
		return 0;
	int written = other.write(buff.data(), avail);
	if (written < 0)
		return 0;
	buff.move(written);
	avail -= written;
	return written;
}

void ts_state::receive() {
	int avail = sock.avail();
	if (avail <= 0)
		throw_sock_error();
	buff.buff.probe(buff.avail + avail);
	if (sock.read(buff.buff.data() + buff.avail, avail) == -1)
		throw_sock_error();
	buff.avail += avail;
}

class bad_request : public std::runtime_error {
public:
	bad_request(const std::string & message) : std::runtime_error(message) {}
};

client::client(tcp_socket_d && sock, worker & w) :
	father(w),
	client_s { state_enum::handshake, std::move(sock) },
	server_s { state_enum::handshake },
	vars(main_vars, srv_vars, f_vars, hs, env_vars) {}

void client::receive_client() {
}

int client::process_request(size_t avail) {
	switch (client_s.state) {
		case state_enum::handshake:
			return handshake(avail);
		case state_enum::ask:
			return -1;
		case state_enum::status:
			return status(avail);
		case state_enum::login:
			return login(avail);
		case state_enum::proxy:
			return proxy(avail);
		default:
			throw std::runtime_error("no such state in process request");
	}
}

int client::handshake(size_t avail) {
	std::int32_t size;
	std::int32_t id;
	int s = pakets::head(client_s.buff, avail, size, id);
	if (s == -1) return -1;
	if (id != pakets::ids::handshake)
		throw bad_request("handshake packet expected");
	s = hs.read(client_s.buff, avail);
	if (s == -1) return -1;
	log_verbose("client " + std::string(client_s.sock.remote_endpoint()) + " send handshake: " + std::to_string(hs));
	conf_snap c = conf;
	const auto & r = record(c);
	if (!r.address.empty() && r.port) {
		try {
			connect(server_s.sock, r.address, r.port);
			pakets::handshake new_hs = hs;
			new_hs.address() = r.address;
			new_hs.port() = r.port;
			log_debug("new handshake " + std::to_string(new_hs));
			int new_hs_len = new_hs.size() + 10;
			extra_buff.probe(new_hs_len);
			int k = hs.write(extra_buff, new_hs_len);
#		ifdef DEBUG
			if (k < 0)
				throw std::runtime_error("FATAL: NO MEMORY IN BUFFER!!!");
#		endif
			extra_buff.avail = k;
			extra_buff.send_to(server_s.sock);
			server_s.sock.set_non_block();
			using namespace actions;
			father.poll.add(server_s.sock, in | et | err | rdhup, [this](descriptor & fd, std::uint32_t events) {
				on_server_sock(fd, events);
			});
			server_s.state = state_enum::ask;
			client_s.state = state_enum::ask;
			return s;
		} catch (const std::exception & e) {
			log_fatal("SHOULD NOT BE HERE");
			//log_verbose("client " + std::string(client_s.sock.remote_endpoint()) + " can't connect to backend server");
			log_debug(e);
		}
	}
	set_state_by_hs();
	return s;
}

int client::status(size_t avail) {
	std::int32_t size;
	std::int32_t id;
	int s = pakets::head(client_s.buff, avail, size, id);
	if (s == -1) return -1;
	switch (id) {
	case pakets::ids::request: {
		pakets::request req;
		s = req.read(client_s.buff, avail);
		if (s == -1) return -1;
		log_debug("client " + std::string(client_s.sock.remote_endpoint()) + " send status request");
		pakets::response res;
		res.message() = resolve_status();
		size_t outsz = res.size() + 10;
		extra_buff.probe(outsz + extra_buff.avail);
		int k = res.write(extra_buff + extra_buff.avail, outsz);
#ifdef DEBUG
		if (k < 0)
			throw paket_error("FATAL: PAKET SIZE != PAKET WRITE");
#endif
		extra_buff.avail += k;
		extra_buff.send_to(client_s.sock);
		return s;
	}
	case pakets::ids::pingpong: {
		pakets::pinpong pp;
		s = pp.read(client_s.buff, avail);
		if (s == -1) return -1;
		log_debug("client " + std::string(client_s.sock.remote_endpoint()) + " send ping pong: " + std::to_string(pp));
		extra_buff.probe(s + extra_buff.avail);
		extra_buff.avail += s;
		std::memcpy(extra_buff + extra_buff.avail, client_s.buff, s);
		extra_buff.send_to(client_s.sock);
		return s;
	}
	default:
		throw paket_error("unexpected packet id: " + std::to_string(id));
	}
}

int client::login(size_t avail) {
	throw bad_request("not implemented yet");
}

int client::proxy(size_t avail) {
	server_s.receive();
	server_s.send_to(client_s);
	return -1;
}

void client::on_server_sock(descriptor &, std::uint32_t events) {
	try {
		if (events & actions::in) {
			switch (server_s.state) {
				case state_enum::ask:
					client_s.state = state_enum::proxy;
					server_s.state = state_enum::proxy;
				case state_enum::proxy:
					server_s.receive();
					server_s.send_to(client_s);
					break;
				default:
					break;
			}
		}
		if (events & actions::out) {
			switch (server_s.state) {
				case state_enum::ask:
					extra_buff.send_to(server_s.sock);
					break;
				case state_enum::proxy:
					client_s.send_to(server_s);
					break;
				default:
					break;
			}
		}
		if (events & actions::err) {
			switch (server_s.state) {
				case state_enum::ask:
					log_debug("backend server is down");
					set_state_by_hs();
					server_s.~ts_state();
					extra_buff.avail = 0;
					break;
				default:
					disconnect();
					return;
			}
		}
		if (events & actions::rdhup) {
			log_verbose("backend server " + std::string(server_s.sock.remote_endpoint()) +
				" disconnect client " + std::string(client_s.sock.remote_endpoint()));
			disconnect();
			return;
		}
	} catch (const std::exception & ex) {
		log_error(ex);
		father.remove(me);
	}
}

std::string client::resolve_status() {
	std::shared_ptr<const config> c = conf;
	std::string name = hs.address().c_str();
	bool allowed = name.size() == hs.address().size();
	auto iter = c->servers.find(name);
	if (iter != c->servers.end()) {
		log_debug("using special config for \"" + name + '"');
		const config::server_record & record = iter->second;
		if (record.allowFML || allowed) {
			srv_vars.vars = &record.vars;
			std::ifstream file(record.status);
			log_debug("open status file: " + record.status);
			if (!file) {
				log_warning("status file '" + record.status + "' not accessible");
				return vars.resolve(res::sample_default_status_json, res::sample_default_status_json_len);
			}
			std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
			return vars.resolve(content);
		}
	}
	const config::server_record & record = c->default_server;
	if (record.allowFML || allowed) {
		srv_vars.vars = &record.vars;
		std::ifstream file(record.status);
		if (!file) {
			log_warning("status file '" + record.status + "' not accessible");
			return vars.resolve(res::sample_default_status_json, res::sample_default_status_json_len);
		}
		std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
		return vars.resolve(content);
	}
	return "";
}

const config::server_record & client::record(const conf_snap & conf) {
	const auto & servers = conf->servers;
	std::string name = hs.address().c_str();
	const auto & iter = servers.find(name);
	bool allowed = name.size() == hs.address().size();
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

void client::disconnect() {
	father.remove(me); // Это как-то грустно...
}

void client::set_state_by_hs() {
	switch (hs.state()) {
	case 1:
		client_s.state = state_enum::status;
		break;
	case 2:
		client_s.state = state_enum::login;
		break;
	default:
		throw bad_request("handshake has a bad state: " + std::to_string(hs.state()));
	}
}

void client::on_client_sock(descriptor &, std::uint32_t events) {
	try {
		if (events & actions::in) {
			client_s.receive();
			while (client_s.buff.avail > 0) {
				log_debug("reminder bytes: " + std::to_string(client_s.buff.avail));
				int s = process_request(client_s.buff.avail);
				if (s < 0)
					return;
				client_s.buff.avail -= s;
				client_s.buff.buff.move(s);
			}
		}
		if (events & actions::out) {
			switch (client_s.state) {
				case state_enum::status:
					extra_buff.send_to(client_s.sock);
					break;
				case state_enum::proxy:
					server_s.send_to(client_s);
					break;
				default:
					break;
			}
		}
		if (events & actions::err) {
			disconnect();
			return;
		}
		if (events & actions::rdhup) {
			log_verbose("client " + std::string(client_s.sock.remote_endpoint()) + " disconnected");
			disconnect();
			return;
		}
	} catch (std::exception & e) {
		log_error("processing client " + std::string(client_s.sock.remote_endpoint()));
		log_error(e);
		disconnect();
	}
}

void client::terminate() {
	// TODO
}

client::~client() {}

} // mcshub
