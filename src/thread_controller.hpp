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
#include <ekutils/event_d.hpp>

#include "client.hpp"

namespace mcshub {

class worker_events final : public ekutils::event_d {
public:
	enum class event_t {
		noop, stop, remove
	};
	inline event_t read() {
		return (event_t)event_d::read();
	}
	inline void write(event_t ev) {
		event_d::write(std::uint64_t(ev));
	}
	worker_events() : event_d(std::int32_t(event_t::noop)) {}
};

class worker final {
	std::future<void> task;
	ekutils::tcp_listener_d listener;
	worker_events events;
	std::list<portal> clients;
	std::atomic<bool> working;
	void on_accept(ekutils::descriptor &, std::uint32_t);
	void on_event(ekutils::descriptor &, std::uint32_t e);
	void job();
public:
	ekutils::epoll_d poll;
	worker();
	std::future<void> & stop();
};

struct thread_controller final {
	static std::uint16_t real_port;
	std::vector<worker> workers;
	thread_controller();
	void terminate();
};

}

#endif // _THREAD_CONTROLLER_HEAD
