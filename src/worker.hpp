#ifndef WORKER_HEAD_PVLKTKWSDFWDC
#define WORKER_HEAD_PVLKTKWSDFWDC

#include <thread>

#include <ekutils/socket_d.hpp>
#include <ekutils/epoll_d.hpp>

#include "portal.hpp"

namespace mcshub {

struct worker_event {
	enum class event_t {
		stop
	};
	virtual event_t type() const noexcept = 0;
	virtual ~worker_event() {}
};

namespace worker_events {
	const inline struct : public worker_event {
		virtual event_t type() const noexcept override {
			return worker_event::event_t::stop;
		}
	} stop;
} // namespace worker_events

typedef std::unique_ptr<ekutils::net::stream_server_socket_d> server_stream_sock;

class worker final {
	ekutils::epoll_d multiplexor;
	std::future<void> task;
	server_stream_sock listener;
	events_base<worker_event> events;
	std::list<portal> clients;
	std::atomic<bool> working;
	void on_accept(ekutils::descriptor &, std::uint32_t);
	void on_event(ekutils::descriptor &, std::uint32_t e);
	void job();
public:
	worker();
	void tell(std::unique_ptr<worker_event> && event) {
		events.write(std::move(event));
	}
	std::future<void> & stop();
};

} // namespace mcshub

#endif // WORKER_HEAD_PVLKTKWSDFWDC
