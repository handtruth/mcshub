#include "server.h"

#include <system_error>
#include <exception>
#include <stdexcept>

#include "log.h"
#include "hardcoded.h"
#include "hosts_db.h"
#include "busy_state.h"
#include "putil.h"

namespace mcshub {

inline void write2sock(tcp_socket_d & sock, const byte_t data[], const size_t amount) {
	// WARNING: THREAD BLOCKING CODE!!!
	for (int k = 0; k < amount; k += sock.write(data + k, amount - k));
}

class bad_request : public std::runtime_error {
public:
	bad_request(const std::string & message) : std::runtime_error(message) {}
};

client::client(tcp_socket_d && so, event & epoll) :
	poll(epoll), sock(std::move(so)), state(state_enum::handshake), rem(0),
	vars(main_vars, srv_vars, f_vars, hs, env_vars), bs(false) {}

void throw_sock_error() {
	int err = errno;
	throw std::system_error(std::make_error_code((std::errc)err));
}

void client::receive() {
	int avail = sock.avail();
	if (avail <= 0)
		throw_sock_error();
	buffer.probe(rem + avail);
	if (sock.read(buffer.data() + rem, avail) == -1)
		throw_sock_error();
	rem += avail;
}

int client::process_request(size_t avail) {
	switch (state) {
		case state_enum::handshake:
			return handshake(avail);
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
	int s = pakets::head(buffer.data(), avail, size, id);
	if (s == -1) return -1;
	if (id != pakets::ids::handshake)
		throw bad_request("handshake packet expected");
	s = hs.read(buffer.data(), avail);
	if (s == -1) return -1;
	log_verbose("client " + std::string(sock.remote_endpoint()) + " send handshake: " + std::to_string(hs));
	std::shared_ptr<const config> c = conf;
	const auto & r = record(c);
	if (!r.address.empty() && r.port) {
		try {
			connect(proxy_sock, r.address, r.port);
			pakets::handshake new_hs = hs;
			new_hs.address() = r.address;
			new_hs.port() = r.port;
			log_debug("new handshake " + std::to_string(new_hs));
			int new_hs_len = new_hs.size() + 10;
			outbuff.probe(new_hs_len);
			int k = hs.write(outbuff.data(), new_hs_len);
#		ifdef DEBUG
			if (k < 0)
				throw std::runtime_error("FATAL: NO MEMORY IN BUFFER!!!");
#		endif
			write2sock(proxy_sock, outbuff.data(), k);
			poll.add(proxy_sock, actions::epoll_in | actions::epoll_rdhup, [this](descriptor &, std::uint32_t events) {
				proxy_backend(events);
			});
			state = state_enum::proxy;
			return s;
		} catch (const std::exception & e) {
			log_verbose("client " + std::string(sock.remote_endpoint()) + " can't connect to backend server");
			log_debug(e);
		}
	}
	switch (hs.state()) {
	case 1:
		state = state_enum::status;
		break;
	case 2:
		state = state_enum::login;
		break;
	default:
		throw bad_request("handshake has a bad state: " + std::to_string(hs.state()));
	}
	return s;
}

int client::status(size_t avail) {
	std::int32_t size;
	std::int32_t id;
	int s = pakets::head(buffer.data(), avail, size, id);
	if (s == -1) return -1;
	switch (id) {
	case pakets::ids::request: {
		pakets::request req;
		s = req.read(buffer.data(), avail);
		if (s == -1) return -1;
		log_debug("client " + std::string(sock.remote_endpoint()) + " send status request");
		pakets::response res;
		res.message() = resolve_status(); // TODO
		size_t outsz = res.size() + 10;
		outbuff.probe(outsz);
		int k = res.write(outbuff.data(), outsz);
#ifdef DEBUG
		if (k < 0)
			throw paket_error("FATAL: PAKET SIZE != PAKET WRITE");
#endif
		write2sock(sock, outbuff.data(), k);
		return s;
	}
	case pakets::ids::pingpong: {
		pakets::pinpong pp;
		s = pp.read(buffer.data(), avail);
		if (s == -1) return -1;
		log_debug("client " + std::string(sock.remote_endpoint()) + " send ping pong: " + std::to_string(pp));
		write2sock(sock, buffer.data(), s);
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
	write2sock(proxy_sock, buffer.data(), avail);
	log_debug("PIPED: to mcshub " + std::to_string(avail));
	return avail;
}

void client::proxy_backend(std::uint32_t events) {
	try {
		if (events & actions::epoll_rdhup) {
			log_verbose("backend server " + std::string(proxy_sock.remote_endpoint()) +
				" disconnect client " + std::string(sock.remote_endpoint()));
			proxy_sock.close();
		} else if (events & actions::epoll_in) {
			size_t avail = proxy_sock.avail();
			if (avail == 0)
				return;
			outbuff.probe(avail);
			log_debug("PIPED: to backend " + std::to_string(avail));
			proxy_sock.read(outbuff.data(), avail);
			write2sock(sock, outbuff.data(), avail);
		}
	} catch (const std::exception & ex) {
		log_verbose("backend server " + std::string(proxy_sock.remote_endpoint()) + " disconnected with error");
		log_debug(ex);
		proxy_sock.close();
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
				return vars.resolve(file_content::default_status);
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
			return vars.resolve(file_content::default_status);
		}
		std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
		return vars.resolve(content);
	}
	return "";
}

const config::server_record & client::record(const std::shared_ptr<const config> & conf) {
	const auto & servers = conf->servers;
	std::string name = hs.address().c_str();
	const auto & iter = servers.find(name);
	bool allowed = name.size() == hs.address().size();
	if (iter != servers.end()) {
		const config::server_record & r = iter->second;
		if (allowed || r.allowFML)
			return r;
	} else {
		const config::server_record & r = conf->default_server;
		if (allowed || r.allowFML)
			return r;
	}
	throw bad_request("modded client is forbidden");
}

bool client::operator()(std::uint32_t events) {
	if (is_busy()) {
		return true;
	}
	busy_state busy(bs);
	try {
		if (events & actions::epoll_rdhup) {
			log_verbose("client " + std::string(sock.remote_endpoint()) + " disconnected");
			return false;
		} else if (events & actions::epoll_in) {
			receive();
			while (rem > 0) {
				log_debug("reminder bytes: " + std::to_string(rem));
				int s = process_request(rem);
				if (s < 0)
					return true;
				rem -= s;
				buffer.move(s);
			}
			return true;
		}
	} catch (std::exception & e) {
		log_error("processing client " + std::string(sock.remote_endpoint()));
		log_error(e);
		return false;
	}
	log_debug("everything OK? I guess...");
	return true;
}

client::~client() {

}

} // mcshub
