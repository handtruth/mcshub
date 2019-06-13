#include "event_pull.h"

#include <cerrno>
#include <stdexcept>
#include <cstring>
#include <string>
#include <system_error>

#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"

namespace mcshub {

void descriptor::close() {
	if (handle != -1) {
		::close(handle);
		handle = -1;
	}
}

descriptor::~descriptor() {
	close();
}

descriptor::operator bool() const {
	return handle != -1;
}

void event_member_base::close() {
	descriptor::close();
	record.reset();
}

event_member_base::record_base::~record_base() {}

signal_d::signal_d(std::initializer_list<sig> signals) {
	sigemptyset(&mask);
	for (sig s : signals)
		sigaddset(&mask, (int) s);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to block signals in mcshub::signal_d constructor");
	handle = signalfd(-1, &mask, 0);
	if (handle < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to create signal fd in mcshub::signal_d constructor");
	// struct signalfd_siginfo fdsi;
}

std::string signal_d::name() const noexcept {
	return "signal";
}

sig signal_d::read() {
	static const ssize_t siginfo_size = sizeof(signalfd_siginfo);
	signalfd_siginfo fdsi;
	ssize_t s = ::read(handle, &fdsi, siginfo_size);
	if (s != siginfo_size)
		throw std::range_error("wrong size of 'signalfd_siginfo' (" + std::to_string(siginfo_size) + " expected, got " + std::to_string(s));
	return sig(fdsi.ssi_signo);
}

signal_d::~signal_d() {}

file_d::file_d(const std::string & path, file_d::mode m) : file(path) {
	handle = open(path.c_str(), int(m));
	if (handle < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to open file \"" + path + "\"");
}

std::string file_d::name() const noexcept {
	return "file (" + file + ")";
}

int file_d::read(byte_t bytes[], size_t length) {
	ssize_t s = ::read(handle, bytes, length);
	if (s < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to read from file \"" + file + "\"");
	return s;
}

int file_d::write(const byte_t bytes[], size_t length) {
	ssize_t s = ::write(handle, bytes, length);
	if (s < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to write to file \"" + file + "\"");
	return s;
}

file_d::~file_d() {}

event::event() {
	handle = epoll_create1(0);
	if (handle < 1)
		throw std::runtime_error(std::string("failed to create epoll: ") + std::strerror(errno));
}

void event::add(event_member_base & fd, event_member_base::record_base * cntxt, std::uint32_t events) {
	fd.record.reset(cntxt);
	epoll_event event;
	event.events = events;
	event.data.ptr = cntxt;
	if (epoll_ctl(handle, EPOLL_CTL_ADD, fd.handle, &event)) {
		throw std::runtime_error(std::string("failed to add file descriptor to epoll: ") + std::strerror(errno));
	}
}

void event::remove(event_member_base & fd) {
	if (epoll_ctl(handle, EPOLL_CTL_DEL, fd.handle, nullptr))
		throw std::runtime_error(std::string("failed to remove file descriptor from epoll: ") + std::strerror(errno));
	fd.record.reset();
}

std::string event::name() const noexcept {
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
		auto & record = *reinterpret_cast<event_member_base::record_base *>(event->data.ptr);
		record(event->events);
		result.push_back(*(record.efd));
	}
	return result;
}

void event::default_action(descriptor & f, std::uint32_t events) {
	log_debug("Cought event: " + f.name());
}

event::~event() {}

}
