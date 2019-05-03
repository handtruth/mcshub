#ifndef _INOTIFY_D_HEAD
#define _INOTIFY_D_HEAD

#include <unordered_map>
#include <vector>
#include <memory>
#include <filesystem>

#include "event_pull.h"

namespace fs = std::filesystem;

namespace mcshub {

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

class inotify_d;

class watch_t final {
private:
	handle_t handle;
	fs::path file;
	std::uint32_t mask;
	inotify_d * fd;
	watch_t(int h, const fs::path & path, std::uint32_t m, inotify_d * ptr) : file(path) {
		handle = h;
		mask = m;
		fd = ptr;
	}
public:
	watch_t(watch_t && other) : handle(other.handle), file(other.file), mask(other.mask), fd(other.fd) {
		other.handle = -1;
		other.fd = nullptr;
	}
	watch_t(const watch_t & other) = delete;
	const fs::path & path() const {
		return file;
	}
	std::uint32_t event_mask() const {
		return mask;
	}
	~watch_t();
	bool operator==(const watch_t & other) const noexcept {
		return handle == other.handle;
	}
	bool operator!=(const watch_t & other) const noexcept {
		return handle != other.handle;
	}
	friend class inotify_d;
};

class inotify_d : public event_member_base {
private:
	std::unordered_map<handle_t, std::shared_ptr<watch_t>> watchers;
	void remove_watch(const std::unordered_map<handle_t, std::shared_ptr<watch_t>>::iterator & iter);
public:
	class event_t {
	private:
		std::shared_ptr<const watch_t> wfd;
	public:
		event_t(const std::shared_ptr<const watch_t> & w, std::uint32_t m) : wfd(w), mask(m) {}
		std::uint32_t mask;
		const watch_t & watch = *wfd;
	private:
		friend class inotify_d;
	};
	inotify_d();
	inotify_d(inotify_d && other);
	const watch_t & add_watch(std::uint32_t mask, const fs::path & path);
	const watch_t & mod_watch(std::uint32_t mask, const watch_t & watch);
	const watch_t & mod_watch(std::uint32_t mask, const std::shared_ptr<const watch_t> & watch) {
		return mod_watch(mask, *watch);
	}
	const watch_t & mod_watch(std::uint32_t mask, const fs::path & path) {
		fs::path canonical = fs::canonical(path);
		for (const auto & pair : watchers) {
			if (pair.second->path() == canonical)
				return mod_watch(mask, pair.second);
		}
		throw std::invalid_argument("no such watch with path \""+ std::string(path) + "\" in inotify object");
	}
	void remove_watch(const watch_t & watch);
	void remove_watch(const std::shared_ptr<const watch_t> & watch) {
		remove_watch(*watch);
	}
	void remove_watch(const fs::path & path);
	const std::vector<std::shared_ptr<const watch_t>> subs() const noexcept;
	virtual std::string name() const noexcept override;
	std::vector<event_t> read();
	friend class watch_t;
};

} // mcshub

#endif // _INOTIFY_D_HEAD
