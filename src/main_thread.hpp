#ifndef MAIN_THREAD_HEAD_APVKVESJCVHKNB
#define MAIN_THREAD_HEAD_APVKVESJCVHKNB

#include <memory>

#include <ekutils/epoll_d.hpp>
#include <ekutils/delegate.hpp>
#include <ekutils/signal_d.hpp>
#include <ekutils/bag.hpp>
#include <ekutils/idgen.hpp>

#include "events.hpp"
#include "manager.hpp"
#include "cli.hpp"

namespace mcshub {

struct main_event {
	enum class event_t {
		callback
	};

	virtual event_t type() const noexcept = 0;
	virtual ~main_event();
};

namespace main_events {

struct callback final : public main_event {
	typedef std::unique_ptr<ekutils::delegate_base<void(void)>> callable_t;
private:
	callable_t callable;
public:
	virtual event_t type() const noexcept override {
		return main_event::event_t::callback;
	}
	callback(callable_t && c) : callable(std::move(c)) {}
	void operator()() {
		(*callable)();
	}
};

} // namespace main_events

class main_thread {
	// resorces
	ekutils::epoll_d multiplexor;
	ekutils::signal_d signals;
	events_base<main_event> events;
	// managers
	ekutils::bag<manager> managers;
	// CLI
	cli commands;

	void react(main_event::event_t type, std::unique_ptr<ekutils::delegate_base<void(const main_event &)>> && action);

public:
	template <typename F>
	void react(main_event::event_t type, F action) {
		react(type, std::make_unique<ekutils::delegate_t<F, void(const main_event &)>(action));
	}
};

} // namespace mcshub

#endif // MAIN_THREAD_HEAD_APVKVESJCVHKNB
