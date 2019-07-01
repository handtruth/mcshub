#include "event_d.h"

#include <system_error>

#include <sys/eventfd.h>

namespace mcshub {

event_d::event_d(unsigned int initval) {
	handle = eventfd(initval, 0);
}

std::uint64_t event_d::read() {
	eventfd_t result;
	if (eventfd_read(handle, &result) == -1)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"error on attempt to read from eventfd");
	return result;
}

void event_d::write(std::uint64_t value) {
	if (eventfd_write(handle, value) == -1)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"error on attempt to write " + std::to_string(value) + " to eventfd");
}

std::string event_d::to_string() const noexcept {
	return "event";
}

}
