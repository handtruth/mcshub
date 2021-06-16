#ifndef SERVER_HEAD_PVCXZZZZCC
#define SERVER_HEAD_PVCXZZZZCC

#include <ekutils/epoll_d.hpp>
#include <ekutils/socket_d.hpp>

#include "portal.hpp"

namespace mcshub {

class server final {
	ekutils::epoll_d & multiplexor;
	std::unique_ptr<ekutils::net::stream_server_socket_d> socket;
	std::list<portal> clients;
};

} // namespace mcshub

#endif // SERVER_HEAD_PVCXZZZZCC
