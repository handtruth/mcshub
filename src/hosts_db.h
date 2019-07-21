#ifndef _HOSTS_DB_HEAD
#define _HOSTS_DB_HEAD

#include <cinttypes>
#include <stdexcept>

#include "socket_d.h"

namespace mcshub {

void connect(tcp_socket_d & socket, const std::string & hostname, std::uint16_t port);

}

#endif // _HOSTS_DB_HEAD
