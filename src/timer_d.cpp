#include "timer_d.h"

#include <sys/timerfd.h>
#include <ctime>
#include <unistd.h>
#include <system_error>
#include <cerrno>
#include <limits>

namespace mcshub {

timer_d::timer_d() {
	handle = timerfd_create(CLOCK_REALTIME, 0);
}

void timer_d::delay(unsigned long secs, unsigned long nanos) {
	timespec now;
	if (clock_gettime(CLOCK_REALTIME, &now) == -1)
		throw std::system_error(std::make_error_code(std::errc(errno)), "error while setting timer delay");
	itimerspec new_value;
	new_value.it_interval.tv_nsec = 0;
	new_value.it_interval.tv_nsec = 0;
	new_value.it_value.tv_nsec = now.tv_sec + static_cast<time_t>(secs);
	new_value.it_value.tv_nsec = now.tv_nsec + static_cast<long>(nanos);
	if (timerfd_settime(handle, 0, &new_value, NULL) == -1)
		throw std::system_error(std::make_error_code(std::errc(errno)), "error while setting timer delay");
}

std::uint64_t timer_d::read() {
	std::uint64_t result;
	int r = ::read(handle, &result, sizeof(result));
	if (r == -1) {
		if (errno == EWOULDBLOCK)
			return std::numeric_limits<std::uint64_t>::max();
		else
			throw std::system_error(std::make_error_code(std::errc(errno)), "error while reading timer status");
	}
	return result;
}

std::string timer_d::to_string() const noexcept {
	return "timer";
}

}
