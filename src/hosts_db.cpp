#include "hosts_db.h"

#include "log.h"

#include <unordered_map>
#include <vector>

namespace mcshub {

std::unordered_map<std::string, std::vector<connection_info>> hosts_db;

void connect(tcp_socket_d & socket, const std::string & hostname, std::uint16_t port) {
	std::string name = (hostname[hostname.length()] == '.') ? hostname.substr(0, hostname.size() - 1) : hostname;
	const auto & iter = hosts_db.find(name);
	if (iter == hosts_db.end()) {
		log_verbose("new dns request for '" + hostname + "'");
		const auto & addresses = hosts_db[name] = std::move(connection_info::resolve(name, port));
		socket.open(addresses, true);
	} else {
		socket.open(iter->second, true);
	}
}

}
