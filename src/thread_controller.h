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
#include "server.h"
#include "socket_d.h"
#include "event_d.h"

namespace mcshub {

class worker_events final : public event_d {
public:
	enum class event_t {
		noop, stop, remove
	};
	struct event {
		virtual event_t type() const noexcept = 0;
		virtual ~event();
	};
private:
	std::deque<event *> fifo;
public:
	struct noop final : public event {
		virtual event_t type() const noexcept override {
			return event_t::noop;
		}
	};
	struct stop final : public event {
		virtual event_t type() const noexcept override {
			return event_t::noop;
		}
	};
	struct remove final : public event {
		virtual event_t type() const noexcept override {
			return event_t::remove;
		}
		std::list<client>::iterator item;
		remove(const std::list<client>::iterator & iter) : item(iter) {}
	};
	inline std::unique_ptr<event> read() {
		std::unique_ptr<event> result;
		if (avail())
			result.reset((event *)event_d::read());
		return result;
	}
	template <typename E, typename ...Args>
	inline void write(Args... value) {
		event * ev = new E(value...);
		if (avail()) {
			log_warning("queue formed for special events, maybe server is overloaded");
			fifo.emplace_back(ev);
		} else {
			event_d::write(std::uint64_t(ev));
		}
	}
	void redo_write_attempt();
};

class worker final {
	std::future<void> task;
	tcp_listener_d listener;
	worker_events events;
	std::list<client> clients;
	std::atomic<bool> working;
	void on_accept(descriptor &, std::uint32_t);
	void on_event(descriptor &, std::uint32_t e);
	void job();
public:
	event poll;
	worker();
	std::future<void> & stop();
	void remove(const std::list<client>::iterator & item);
};

struct thread_controller final {
	std::vector<worker> workers;
	thread_controller();
	void terminate();
};

}

#endif // _THREAD_CONTROLLER_HEAD
