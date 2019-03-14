#include <cinttypes>
#include <csignal>
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

namespace mcshub {

typedef int handle_t;

class event;

class descriptor {
public:
	descriptor(const descriptor & other) = delete;
	descriptor(descriptor && other) : handle(other.handle) {
		other.handle = -1;
	}
	virtual std::string name() const noexcept = 0;
	virtual void close();
	virtual ~descriptor();
	bool operator==(const descriptor & other) const noexcept {
		return handle == other.handle;
	}
	virtual operator bool() const;
protected:
	descriptor() {}
	handle_t handle;
	
	friend class event;
};

class event_member_base : public descriptor {
public:
	event_member_base() {}
	event_member_base(event_member_base && other) : descriptor(static_cast<descriptor &&>(other)) {
		record.reset(other.record.release());
		if (record) {
			record->efd = this;
		}
	}
	virtual void close() override;
private:
	struct record_base {
		descriptor * efd;
		record_base(record_base && other) = delete;
		record_base(const record_base & other) = delete;
		record_base(descriptor & fd) : efd(&fd) {}
		virtual void operator()(std::uint32_t events) = 0;
		virtual ~record_base();
	};
	template <typename F>
	struct record_t : public record_base {
		F act;
		record_t(descriptor & fd, F action) : record_base(fd), act(action) {}
		virtual void operator()(std::uint32_t events) override {
			act(*efd, events);
		}
	};
private:
	std::unique_ptr<record_base> record;
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

class signal_d : public event_member_base {
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

class inotify_d : public event_member_base {
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

class file_d : public event_member_base {
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
	static void default_action(descriptor & f, std::uint32_t);
	void add(event_member_base & fd, event_member_base::record_base * cntxt, std::uint32_t events);
public:
	event();
	template <typename F>
	void add(event_member_base & fd, std::uint32_t events, F action) {
		auto cntxt = new event_member_base::record_t<F>(fd, action);
		add(fd, cntxt, events);
	}
	template <typename F>
	void add(event_member_base & fd, F action) {
		auto cntxt = new event_member_base::record_t<F>(fd, action);
		add(fd, cntxt, actions::epoll_in);
	}
	void remove(event_member_base & fd);
	virtual std::string name() const noexcept override;
	std::vector<std::reference_wrapper<descriptor>> read(int timeout = 0);

	virtual ~event() override;
};

} // mcshub
