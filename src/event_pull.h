#include <cinttypes>
#include <csignal>
#include <initializer_list>
#include <string>
#include <vector>
#include <functional>

#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "primitives.h"

namespace mcshub {

typedef int handle_t;

class event;

class descriptor {
public:
	descriptor(const descriptor & other) = delete;
	descriptor(descriptor && other) = delete;
	virtual std::string name() const noexcept = 0;
	virtual ~descriptor() {}
protected:
	descriptor() {}
	handle_t handle;
	bool operator==(const descriptor & other) const noexcept {
		return handle == other.handle;
	}
	
	friend class event;
};

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

class signal_d : public descriptor {
private:
	sigset_t mask;
public:
	signal_d(std::initializer_list<sig> signals);
	virtual std::string name() const noexcept override;
	sig read();
	virtual ~signal_d() override;
};

namespace inev {
	enum inev_t : std::uint32_t {
		access        = IN_ACCESS,
		attrib        = IN_ATTRIB,
		close_write   = IN_CLOSE_WRITE,
		close_nowrite = IN_CLOSE_NOWRITE,
		create        = IN_CREATE,
		in_delete     = IN_DELETE,
		delete_self   = IN_DELETE_SELF,
		modify        = IN_MODIFY,
		move_self     = IN_MOVE_SELF,
		moved_from    = IN_MOVED_FROM,
		moved_to      = IN_MOVED_TO,
		open          = IN_OPEN,

		// Combined
		move          = IN_MOVE,
		close         = IN_CLOSE,
		all           = IN_ALL_EVENTS,
	};
}

class watch_d;

class inotify_d : public descriptor {
private:
	std::vector<watch_d> watchers;
public:
	struct ino_event {
		const watch_d & watch;
		inev::inev_t mask;
	private:
		ino_event(const watch_d & w, inev::inev_t m) : watch(w), mask(m) {}
		friend inotify_d;
	};
	inotify_d();
	const watch_d & add_watch(inev::inev_t mask, const std::string & path);
	virtual std::string name() const noexcept override;
	std::vector<ino_event> read();
	virtual ~inotify_d() override;
};

class watch_d : public descriptor {
private:
	std::string file;
	inev::inev_t mask;
	watch_d(int h, const std::string & file, inev::inev_t m) {
		handle = h;
		this->file = file;
		mask = m;
	}
public:
	watch_d(mcshub::watch_d&& other) : file(std::move(other.file)) {
		handle = other.handle;
		mask = other.mask;
	}
	virtual std::string name() const noexcept override;
	const std::string & path() const {
		return file;
	}
	inev::inev_t event_mask() const {
		return mask;
	}
	bool operator==(const watch_d & other) const noexcept {
		return handle == other.handle;
	}
	bool operator!=(const watch_d & other) const noexcept {
		return handle != other.handle;
	}
	friend class inotify_d;
};

class file_d : public descriptor {
private:
	std::string file;
public:
	enum class mode : int {
		ro = O_RDONLY,
		wo = O_WRONLY,
		wr = O_RDWR,
		aw = O_APPEND,
	};
	file_d(const std::string & path, mode m);
	virtual std::string name() const noexcept override;
	int read(byte_t bytes[], size_t length);
	int write(const byte_t bytes[], size_t length);
	virtual ~file_d() override;
};

namespace actions {
	enum actions_t : uint32_t {
		epoll_in        = EPOLLIN,
		epoll_out       = EPOLLOUT,
		epoll_rdhup     = EPOLLRDHUP,
		epoll_rpi       = EPOLLPRI,
		epoll_err       = EPOLLERR,
		epoll_hup       = EPOLLHUP,
		epoll_et        = EPOLLET,
		epoll_oneshot   = EPOLLONESHOT,
		epoll_wakeup    = EPOLLWAKEUP,
		epoll_exclusive = EPOLLEXCLUSIVE
	};
}

class event : public descriptor {
private:
	struct record_t {
		descriptor & d;
		std::function<void(descriptor &, std::uint32_t)> action;
		epoll_event ud;
		record_t(record_t && other) = delete;
		record_t(const record_t & other) = delete;
		record_t(descriptor & f, const std::function<void(descriptor &, std::uint32_t)> & act, std::uint16_t events) : d(f) {
			action = act;
			ud.data.ptr = this;
			ud.events = events;
		}
	};
	std::vector<record_t *> data;
	static void default_action(descriptor & f, std::uint32_t);
	static const std::function<void(descriptor &, std::uint32_t)> default_action_p;
public:
	event();
	void add(descriptor & f, std::uint32_t events = actions::epoll_in, const std::function<void(descriptor &, std::uint32_t)> & action = default_action_p);
	void remove(descriptor & f);
	virtual std::string name() const noexcept override;
	std::vector<std::reference_wrapper<descriptor>> read(int timeout = 0);

	~event();
};

} // mcshub
