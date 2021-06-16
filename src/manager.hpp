#ifndef MANAGER_HEAD_SPPVKRKFJNKW
#define MANAGER_HEAD_SPPVKRKFJNKW

#include <ekutils/uri.hpp>
#include <ekutils/socket_d.hpp>
#include <ekutils/ip_filter.hpp>
#include <ekutils/epoll_d.hpp>

namespace mcshub {

class manager {
	ekutils::epoll_d & multiplexer;
	ekutils::net::ip_filter filter;
	std::unique_ptr<ekutils::net::datagram_server_socket_d> socket;

public:
	manager(ekutils::epoll_d & poll, const ekutils::uri & uri);
	void start();
	~manager();
};

} // namespace mcshub

#endif // MANAGER_HEAD_SPPVKRKFJNKW
