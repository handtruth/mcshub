#include "event_pull.h"

#include <cerrno>
#include <stdexcept>
#include <cstring>
#include <string>
#include <system_error>

#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"

namespace mcshub {

event::event() {
	handle = epoll_create1(0);
	if (handle < 1)
		throw std::runtime_error(std::string("failed to create epoll: ") + std::strerror(errno));
}

void event::add(descriptor & fd, descriptor::record_base * cntxt, std::uint32_t events) {
	fd.record.reset(cntxt);
	epoll_event event;
	event.events = events;
	event.data.ptr = cntxt;
	if (epoll_ctl(handle, EPOLL_CTL_ADD, fd.handle, &event)) {
		throw std::runtime_error(std::string("failed to add file descriptor to epoll: ") + std::strerror(errno));
	}
}

void event::remove(descriptor & fd) {
	if (epoll_ctl(handle, EPOLL_CTL_DEL, fd.handle, nullptr))
		throw std::runtime_error(std::string("failed to remove file descriptor from epoll: ") + std::strerror(errno));
	fd.record.reset();
}

std::string event::to_string() const noexcept {
	return "event poll";
}

std::vector<std::reference_wrapper<descriptor>> event::pull(int timeout) {
	static const int max_event_n = 7;
	epoll_event events[max_event_n];
	int catched = epoll_wait(handle, events, max_event_n, timeout);
	if (catched < 0)
		throw std::runtime_error(std::string("failed to catch events from event pull: ") + std::strerror(errno));
	std::vector<std::reference_wrapper<descriptor>> result;
	for (int i = 0; i < catched; i++) {
		auto event = &events[i];
		auto & record = *reinterpret_cast<descriptor::record_base *>(event->data.ptr);
		record(event->events);
		result.push_back(*(record.efd));
	}
	return result;
}

void event::default_action(descriptor & f, std::uint32_t events) {
	log_debug("Cought event: " + f.to_string());
}

event::~event() {}

}
