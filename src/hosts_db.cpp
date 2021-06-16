#include "hosts_db.hpp"

#include <unordered_map>

#include <ekutils/log.hpp>
#include <ekutils/resolver.hpp>

#include "mcshub_arguments.hpp"

namespace mcshub {

typedef std::forward_list<ekutils::net::resolution> resolutions;

std::unordered_map<std::string, resolutions> hosts_db;

inline resolutions resolve(const ekutils::uri & uri) {
	return ekutils::net::resolve(ekutils::net::socket_types::stream, uri, static_cast<std::uint8_t>(arguments.default_port));
}

const resolutions & request_dns_cache(const std::string & address) {
	const auto & iter = hosts_db.find(address);
	if (iter == hosts_db.end()) {
		log_verbose("new dns cache request for '" + address + "'");
		const auto & addresses = hosts_db.emplace(address, resolve(address));
		return addresses.first->second;
	} else {
		return iter->second;
	}
}

stream_sock connect(bool use_cache, const std::string & address) {
	resolutions tmp = use_cache ? resolutions() : resolve(address);
	const resolutions & targets = use_cache ? request_dns_cache(address) : tmp;
	return ekutils::net::connect_any(targets.begin(), targets.end(), ekutils::net::socket_flags::non_block);
}

}
