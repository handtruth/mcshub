#ifndef _EVENT_PULL_HEAD
#define _EVENT_PULL_HEAD

#include <cinttypes>
#include <initializer_list>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "primitives.h"
#include "descriptor.h"

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
	static void default_action(descriptor & f, std::uint32_t);
	void add(descriptor & fd, descriptor::record_base * cntxt, std::uint32_t events);
public:
	event();
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
	std::vector<std::reference_wrapper<descriptor>> pull(int timeout = 0);
	virtual ~event() override;
};

} // mcshub

#endif // _EVENT_PULL_HEAD
