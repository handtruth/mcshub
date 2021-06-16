#include "control.hpp"

#include <sstream>

#include <ekutils/log.hpp>

#include "protocol.hpp"
#include "connector.hpp"
#include "settings.hpp"
#include "prog_args.hpp"

namespace mcshub {

control_port::control_port(ekutils::epoll_d & epoll_d, const std::string & link) :
	epoll(epoll_d)
{
	uri uri(link);
	server = listener(uri, ekutils::sock_flags::non_blocking);
	const auto & parameters = uri.get_query_dictionary();
	auto iter = parameters.find("allow");
	if (iter != parameters.cend()) {
		std::stringstream stream(iter->second);
		while (stream.good()) {
			std::string substr;
			std::getline(stream, substr, ',');
			whitelist.push_back(substr);
		}
	}
	server->start(2);
	epoll.add(*server, [this](const auto &, const auto &) {
		auto sock = server->accept_virtual();
		sock->set_non_block();
		auto client = new control_client(epoll, std::move(sock));
		log_verbose("control " + std::string(client->sock().remote_endpoint()) + " connected");
		using namespace ekutils::actions;
		epoll.add(client->sock(), in | out | et | err | rdhup, [this, client](ekutils::descriptor &, std::uint32_t events) {
			client->on_event(events);
			if (client->is_disconnected()) {
				log_verbose("control " + std::string(client->sock().remote_endpoint()) + " disconnected");
				epoll.remove(client->sock());
				delete client;
			}
		});
	});
}

control_port::~control_port() {
	epoll.remove(*server);
}

ekutils::idgen<int> control_client::ids = ekutils::idgen<int>();

control_client::control_client(ekutils::epoll_d & epoll_d, stream_sock && c) :
	epoll(epoll_d), passage(std::move(c)), id(ids.next()), disconnected(false), connected(false) {}

void control_client::on_event(std::uint32_t events) {
	using namespace ekutils;
	try {
		if (events & actions::rdhup) {
			disconnect();
		}
		if (events & actions::err) {
			// Disconnect with async error
			int err = errno;
			log_debug("async connection error received from control #" + std::to_string(id));
			throw std::system_error(std::make_error_code(std::errc(err)), "async error");
		}
		if (events & actions::in) {
			// New data for work has received
			passage.receive();
			process_request();
		}
		if (events & actions::out) {
			// Out information is ready to be send
			passage.send();
		}
	} catch (const std::exception & e) {
		log_error("connection error on client #" + std::to_string(id));
		log_error(e);
		disconnect();
	}
}

void control_client::process_request() {
	if (connected)
		while (process_next());
	else
		process_handshake();
}

void control_client::process_handshake() {
	protocol::handshake hs;
	passage.paket_read(hs);
	if (hs.version() != protocol::version)
		throw bad_request("bad version number: " + std::to_string(hs.version()));
	connected = true;
	while (process_next());
}

#define phead(id, size) \
		do { if (!passage.head(id, size)) return false; } while (0)

#define pread(p) \
		do { if (!passage.paket_read(p)) return false; } while (0)

#define pwrite(p) \
		(passage.paket_write(p))

bool control_client::process_next() {
	std::int32_t id, size;
	phead(id, size);
	switch (id) {
	case protocol::ids::record: {
		protocol::record record;
		pread(record);
		conf_snap conf;
		std::map<std::string, std::string> vars;
		auto size = std::min(record.vars_keys().size(), record.vars_values().size());
		for (decltype(size) i = 0; i < size; i++)
			vars.emplace(record.vars_keys()[i], record.vars_values()[i]);
		settings::basic_record srv_record {
			std::move(record.address()), record.port() ? record.port() : arguments.default_port,
			strparts(std::move(record.status())), strparts(std::move(record.login())), false, false,
			//std::move(vars)
			{}
		};
		//if (record.name().empty())
		break;
	}
	case protocol::ids::drop_record: {
		//protocol::reco
		break;
	}
	case protocol::ids::no_record:
		break;
	case protocol::ids::get_record:
		break;
	default:
		throw bad_request("unknown paket id: " + std::to_string(id));
	}
	return true;
}

} // namespace mcshub
