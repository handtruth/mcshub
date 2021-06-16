#include "manager.hpp"

#include <ekutils/resolver.hpp>

namespace mcshub {

manager::manager(ekutils::epoll_d & poll, const ekutils::uri & uri) : multiplexer(poll) {
	auto targets = ekutils::net::resolve(ekutils::net::socket_types::datagram, uri);
	socket = ekutils::net::bind_datagram_any(targets.begin(), targets.end(), ekutils::net::socket_flags::non_block);
	const auto & options = uri.get_query_dictionary();
	auto allowed = options.equal_range("allow");
	for (auto iter = allowed.first; iter != allowed.second; ++iter) {
		filter.allow(iter->second);
	}
	auto denied = options.equal_range("deny");
	for (auto iter = denied.first; iter != denied.second; ++iter) {
		filter.deny(iter->second);
	}
}

void manager::start() {
	
}

} // namespace mcshub
