#ifndef _THREAD_CONTROLLER_HEAD
#define _THREAD_CONTROLLER_HEAD

#include <future>
#include <list>
#include <vector>
#include <atomic>
#include <memory>
#include <deque>

#include "event_pull.h"
#include "primitives.h"
#include "client.h"
#include "socket_d.h"
#include "event_d.h"

namespace mcshub {

class worker_events final : public event_d {
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
	tcp_listener_d listener;
	worker_events events;
	std::list<portal> clients;
	std::atomic<bool> working;
	void on_accept(descriptor &, std::uint32_t);
	void on_event(descriptor &, std::uint32_t e);
	void job();
public:
	event poll;
	worker();
	std::future<void> & stop();
};

struct thread_controller final {
	std::vector<worker> workers;
	thread_controller();
	void terminate();
};

}

#endif // _THREAD_CONTROLLER_HEAD
