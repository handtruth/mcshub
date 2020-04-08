#ifndef _HOSTS_DB_HEAD
#define _HOSTS_DB_HEAD

#include <cinttypes>
#include <stdexcept>

#include <ekutils/socket_d.hpp>

namespace mcshub {

void connect(ekutils::tcp_socket_d & socket, bool cache,
    const std::string & hostname, std::uint16_t port);

}

#endif // _HOSTS_DB_HEAD
