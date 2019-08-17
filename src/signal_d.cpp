#include "signal_d.h"

#include <system_error>
#include <sys/signalfd.h>
#include <unistd.h>
#include <signal.h>

namespace mcshub {

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

std::string signal_d::to_string() const noexcept {
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

}
