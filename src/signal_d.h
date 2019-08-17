#ifndef _SIGNAL_D_HEAD
#define _SIGNAL_D_HEAD

#include <csignal>
#include <string>

#include "descriptor.h"

namespace mcshub {

enum class sig {
	hangup              = SIGHUP,
	interrupt           = SIGINT,
	quit                = SIGQUIT,
	illegal_instruction = SIGILL,
	abort               = SIGABRT,
	float_exception     = SIGFPE,
	kill                = SIGKILL,
	segmentation_fail   = SIGSEGV,
	broken_pipe         = SIGPIPE,
	alarm               = SIGALRM,
	termination         = SIGTERM,
	user1               = SIGUSR1,
	user2               = SIGUSR2,
	child_stopped       = SIGCHLD,
	cont                = SIGCONT,
	stop                = SIGSTOP,
	typed_stop          = SIGTSTP,
	input               = SIGTTIN,
	output              = SIGTTOU,

	bus                 = SIGBUS,
	poll                = SIGPOLL,
	profiling_expired   = SIGPROF,
	bad_sys_call        = SIGSYS,
	trap                = SIGTRAP,
	urgent_condition    = SIGURG,
	virtual_alarm       = SIGVTALRM,
	cpu_time_exceeded   = SIGXCPU,
	file_size_limit     = SIGXFSZ,
};

inline int kill(pid_t pid, sig signal) {
	return::kill(pid, (int) signal);
}

class signal_d : public descriptor {
private:
	sigset_t mask;
public:
	signal_d(std::initializer_list<sig> signals);
	virtual std::string to_string() const noexcept override;
	sig read();
	virtual ~signal_d() override;
};

} // namespace mcshub

#endif // _SIGNAL_D_HEAD
