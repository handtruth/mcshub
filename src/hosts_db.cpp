#include "hosts_db.hpp"

#include <unordered_map>
#include <vector>

#include <ekutils/log.hpp>

namespace mcshub {

std::unordered_map<std::string, std::vector<ekutils::connection_info>> hosts_db;

void connect(ekutils::tcp_socket_d & socket, const std::string & hostname, std::uint16_t port) {
	std::size_t l = hostname.length() - 1;
	std::string name = (hostname[l] == '.') ? hostname.substr(0, l) : hostname;
	const auto & iter = hosts_db.find(name);
	if (iter == hosts_db.end()) {
		log_verbose("new dns request for '" + hostname + "'");
		const auto & addresses = hosts_db[name] = std::move(ekutils::connection_info::resolve(name, port));
		socket.open(addresses, ekutils::tcp_flags::non_blocking);
	} else {
		socket.open(iter->second, ekutils::tcp_flags::non_blocking);
	}
}

}
