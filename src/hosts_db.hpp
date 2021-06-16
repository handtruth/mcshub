#ifndef _HOSTS_DB_HEAD
#define _HOSTS_DB_HEAD

#include <cinttypes>
#include <stdexcept>

#include <ekutils/socket_d.hpp>

#include "gate.hpp"

namespace mcshub {

stream_sock connect(bool use_cache, const std::string & address);

}

#endif // _HOSTS_DB_HEAD
