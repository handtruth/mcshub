#include "hosts_db.hpp"

#include <unordered_map>
#include <vector>

#include <ekutils/log.hpp>

namespace mcshub {

std::unordered_map<std::string, std::vector<ekutils::connection_info>> hosts_db;

const std::vector<ekutils::connection_info> & request_dns_cache(
		const std::string & hostname, std::uint16_t port) {
	const auto & iter = hosts_db.find(hostname);
	if (iter == hosts_db.end()) {
		log_verbose("new dns cache request for '" + hostname + "'");
		const auto & addresses = hosts_db[hostname] = std::move(ekutils::connection_info::resolve(hostname, port));
		return addresses;
	} else {
		return iter->second;
	}
}

void connect(ekutils::tcp_socket_d & socket, bool cache,
		const std::string & hostname, std::uint16_t port) {
	std::size_t l = hostname.length() - 1;
	std::string name = (hostname[l] == '.') ? hostname.substr(0, l) : hostname;
	if (cache)
		socket.open(request_dns_cache(name, port), ekutils::tcp_flags::non_blocking);
	else
		socket.open(ekutils::connection_info::resolve(name, port), ekutils::tcp_flags::non_blocking);
}

}
