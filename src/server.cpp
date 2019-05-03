#include "server.h"

#include <system_error>
#include <exception>
#include <stdexcept>

#include "log.h"
#include "hardcoded.h"
#include "settings.h"

namespace mcshub {

class bad_request : public std::runtime_error {
public:
	bad_request(const std::string & message) : std::runtime_error(message) {}
};
//vars_manager<main_vars_t, server_vars, file_vars, pakets::handshake, env_vars_t>
client::client(tcp_socket_d && so, event & epoll) :
	poll(epoll), sock(std::move(so)), state(state_enum::handshake), rem(0),
	vars(main_vars, srv_vars, f_vars, hs, env_vars) {}

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
	//log->fatal("2 == " + std::to_string(s));
	if (s == -1) return -1;
	if (id != pakets::ids::handshake)
		throw bad_request("handshake packet expected");
	s = hs.read(buffer.data(), avail);
	//log->fatal("WAS HERE: " + std::to_string(s) + ":" + std::to_string(avail) + " size:" + std::to_string(size));
	if (s == -1) return -1;
	log->verbose("client " + std::string(sock.remote_endpoint()) +
		" send handshake: " + std::to_string(hs));
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

std::string client::resolve_status() {
	std::shared_ptr<const config> c = conf;
	std::string name = hs.address().c_str();
	bool allowed = name.size() == hs.address().size();
	auto iter = c->servers.find(name);
	if (iter != c->servers.end()) {
		const config::server_record & record = iter->second;
		if (record.allowFML) {
			srv_vars.vars = &record.vars;
			std::ifstream file(record.status);
			if (!file) {
				log->warning("status file '" + record.status + "' not accessible");
				return vars.resolve(file_content::default_status);
			}
			std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
			return vars.resolve(content);
		}
	}
	const config::server_record & record = c->default_server;
	if (record.allowFML) {
		srv_vars.vars = &record.vars;
		std::ifstream file(record.status);
		if (!file) {
			log->warning("status file '" + record.status + "' not accessible");
			return vars.resolve(file_content::default_status);
		}
		std::string content = std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
		return vars.resolve(content);
	}
	return "";
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
		log->debug("client " + std::string(sock.remote_endpoint()) + " send status request");
		pakets::response res;
		res.message() = resolve_status(); // TODO
		size_t outsz = res.size() + 10;
		outbuff.probe(outsz);
		int k = res.write(outbuff.data(), outsz);
#ifdef DEBUG
		if (k < 0)
			throw paket_error("FATAL: PAKET SIZE != PAKET WRITE");
#endif
		k = sock.write(outbuff.data(), outsz);
		if (k == -1)
			throw_sock_error();
		return s;
	}
	case pakets::ids::pingpong: {
		pakets::pinpong pp;
		s = pp.read(buffer.data(), avail);
		if (s == -1) return -1;
		log->debug("client " + std::string(sock.remote_endpoint()) +
			" send ping pong: " + std::to_string(pp));
		int k = sock.write(buffer.data(), s);
		if (k == -1)
			throw_sock_error();
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
	throw bad_request("not implemented yet");
}

bool client::operator()(std::uint32_t events) {
	try {
		if (events & actions::epoll_rdhup) {
			log->verbose("client " + std::string(sock.remote_endpoint()) + " disconnected");
			return false;
		} else if (events & actions::epoll_in) {
			receive();
			while (rem > 0) {
				log->debug("reminder bytes: " + std::to_string(rem));
				int s = process_request(rem);
				if (s < 0)
					return true;
				rem -= s;
				buffer.move(s);
			}
			return true;
		}
	} catch (std::exception & e) {
		log->error("processing client " + std::string(sock.remote_endpoint()));
		log->error(e);
		return false;
	}
	log->debug("everything OK? I guess...");
	return true;
}

client::~client() {

}

} // mcshub
