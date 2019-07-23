#ifndef _EVENT_PULL_HEAD
#define _EVENT_PULL_HEAD

#include <cinttypes>
#include <initializer_list>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <chrono>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "primitives.h"
#include "descriptor.h"
#include "delegate.h"

namespace mcshub {

namespace actions {
	enum actions_t : uint32_t {
		in        = EPOLLIN,
		out       = EPOLLOUT,
		rdhup     = EPOLLRDHUP,
		rpi       = EPOLLPRI,
		err       = EPOLLERR,
		hup       = EPOLLHUP,
		et        = EPOLLET,
		oneshot   = EPOLLONESHOT,
		wakeup    = EPOLLWAKEUP,
		exclusive = EPOLLEXCLUSIVE
	};
}

class event : public descriptor {
private:
	struct event_timer {
		int timeout;
		std::unique_ptr<delegate_base<void()>> action;
		bool sudden = true;
		template <typename F>
		event_timer(int span, F act) :
			timeout(span),
			action(std::make_unique<delegate_t<F, void()>>(act)) {}
	};
	struct timer_cmp {
		inline bool operator()(const event_timer & a, const event_timer & b) {
			return a.timeout < b.timeout;
		}
	};
	struct event_queue : public std::priority_queue<event_timer, std::vector<event_timer>, timer_cmp> {
		std::vector<event_timer> & container() noexcept {
			return c;
		}
	};
	event_queue timers;
	static void default_action(descriptor & f, std::uint32_t);
	void add(descriptor & fd, descriptor::record_base * cntxt, std::uint32_t events);
public:
	event();
	template <typename F, typename Rep, typename Period>
	void later(std::chrono::duration<Rep, Period> span, F action) {
		using namespace std::chrono;
		timers.emplace(static_cast<int>(duration_cast<milliseconds>(span).count()), action);
	}
	template <typename F>
	void add(descriptor & fd, std::uint32_t events, F action) {
		auto cntxt = new descriptor::record_t<F>(fd, action);
		add(fd, cntxt, events);
	}
	template <typename F>
	void add(descriptor & fd, F action) {
		auto cntxt = new descriptor::record_t<F>(fd, action);
		add(fd, cntxt, actions::in);
	}
	void remove(descriptor & fd);
	virtual std::string to_string() const noexcept override;
	void pull(int timeout = 0);
	virtual ~event() override;
	friend class std::priority_queue<event_timer, std::vector<event_timer>, timer_cmp>;
};

} // mcshub

#endif // _EVENT_PULL_HEAD
