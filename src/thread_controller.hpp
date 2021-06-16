#ifndef _THREAD_CONTROLLER_HEAD
#define _THREAD_CONTROLLER_HEAD

#include <future>
#include <list>
#include <vector>
#include <atomic>
#include <memory>
#include <deque>

#include <ekutils/epoll_d.hpp>
#include <ekutils/socket_d.hpp>

#include "events.hpp"
#include "worker.hpp"
#include "portal.hpp"

namespace mcshub {

struct thread_controller final {
	static ekutils::net::port_t real_port;
	std::vector<worker> workers;
	thread_controller();
	void terminate();
};

}

#endif // _THREAD_CONTROLLER_HEAD
