#include "inotify_d.h"

#include <sys/inotify.h>
#include <unistd.h>

#include "log.h"

namespace mcshub {

watch_t::~watch_t() {
	
}

void inotify_d::remove_watch(const std::unordered_map<handle_t, std::shared_ptr<watch_t>>::iterator & iter) {
	inotify_rm_watch(handle, iter->first);
	iter->second->handle = 0;
	watchers.erase(iter);
}

inotify_d::inotify_d() {
	handle = inotify_init();
	if (handle < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to create inotify in mcshub::inotify_d constructor");
}

inotify_d::inotify_d(inotify_d && other) : descriptor(static_cast<inotify_d &&>(other)), watchers(std::move(other.watchers)) {
	for (auto & pair : watchers)
		pair.second->fd = this;
}

const watch_t & inotify_d::add_watch(std::uint32_t mask, const fs::path & path, watch_t::data_t * data) {
	fs::path canonical = fs::weakly_canonical(path);
	int h = inotify_add_watch(handle, canonical.c_str(), mask);
	if (h < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to add watcher to inotify in mcshub::inotify_d::add_watch (path is \""
				+ std::string(canonical) + "\", watch mask " + std::to_string(mask) + ")");
	std::shared_ptr<watch_t> watch = std::make_shared<watch_t>(watch_t(h, canonical, mask, this));
	watch->data = data;
	watchers[h] = watch;
	return *watch;
}

const watch_t & inotify_d::mod_watch(std::uint32_t mask, const watch_t & watch) {
	if (watch.fd != this)
		throw std::invalid_argument("this watch does not belong to this inotify object");
	int h = inotify_add_watch(handle, watch.file.c_str(), mask);
	if (h < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to modify watch in inotify (path is \""
				+ std::string(watch.path()) + "\", new watch mask " + std::to_string(mask) + ")");
	if (h != watch.handle)
		throw std::runtime_error("FATAL new watch created! (expected to modify old one)");
	watchers[h]->mask = mask;
	return watch;
}

void inotify_d::remove_watch(const watch_t & watch) {
	for (auto it = watchers.begin(), end = watchers.end(); it != end; it++)
		if (it->first == watch.handle) {
			watchers.erase(it);
			return;
		}
	throw std::invalid_argument("no such filesystem watch in inotify: " + std::string(watch.path()));
}

void inotify_d::remove_watch(const fs::path & path) {
	fs::path full = fs::weakly_canonical(path);
	for (auto it = watchers.begin(), end = watchers.end(); it != end; it++)
		if (it->second->file == full) {
			watchers.erase(it);
			return;
		}
	throw std::invalid_argument("no such filesystem watch in inotify: " + std::string(path));
}

const std::vector<std::shared_ptr<const watch_t>> inotify_d::subs() const noexcept {
	std::vector<std::shared_ptr<const watch_t>> result;
	for (const auto & pair : watchers)
		result.emplace_back(pair.second);
	return result;
}

std::string inotify_d::to_string() const noexcept {
	return "inotify";
}

std::vector<inotify_d::event_t> inotify_d::read() {
	static const ssize_t buff_size = 50*(sizeof(inotify_event) + 16);
	byte_t buffer[buff_size];
	std::vector<event_t> result;
	ssize_t length = ::read(handle, buffer, buff_size);
	if (length < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to read inotify fd in mcshub::inotify_d::read");
	ssize_t i = 0;
	while (i < length) {
		inotify_event *event = reinterpret_cast<inotify_event *>(buffer + i);
		std::shared_ptr<const watch_t> watch;
		for (const auto & pair : watchers) {
			if (pair.first == event->wd)
				watch = pair.second;
		}
		if (watch == nullptr)
			throw std::runtime_error("inotify watcher isn't present in set");
		result.emplace_back(watch, event->mask, event->name);
		i += sizeof(inotify_event) + event->len;
	}
	return result;
}

} // mcshub
