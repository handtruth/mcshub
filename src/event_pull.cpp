#include "event_pull.h"

#include <cerrno>
#include <stdexcept>
#include <cstring>
#include <string>
#include <system_error>

#include <sys/signalfd.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"

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

signal_d::~signal_d() { 
    close(handle);
}

inotify_d::inotify_d() {
    handle = inotify_init();
    if (handle < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to create inotify in mcshub::inotify_d constructor");
}

const watch_d & inotify_d::add_watch(inev::inev_t mask, const std::string & path) {
    int h = inotify_add_watch(handle, path.c_str(), mask);
    if (h < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to add watcher to inotify in mcshub::inotify_d::add_watch (path is \""
                + path + "\", watch mask " + std::to_string(mask) + ")");
    watchers.emplace_back(watch_d(h, path, mask));
    return watchers.back();
}

std::string inotify_d::name() const noexcept {
    return "inotify";
}

std::vector<inotify_d::ino_event> inotify_d::read() {
    static const ssize_t buff_size = 50*(sizeof(inotify_event) + 16);
    byte_t buffer[buff_size];
    std::vector<ino_event> result;
    ssize_t length = ::read(handle, buffer, buff_size);
    if (length < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to read inotify fd in mcshub::inotify_d::read");
    ssize_t i = 0;
    while(i < length) {
        inotify_event *event = reinterpret_cast<inotify_event *>(buffer + i);
        if (event->len) {
            const watch_d * watch = nullptr;
            for (const watch_d & w : watchers) {
                if (w.handle == event->wd)
                    watch = &w;
            }
            if (watch == nullptr)
                throw std::runtime_error("inotify watcher isn't present in set (mcshub::inotify_d::read)");
            result.emplace_back(ino_event(*watch, inev::inev_t(event->mask)));
        }
        i += sizeof(inotify_event) + event->len;
    }
    return result;
}

inotify_d::~inotify_d() {
    close(handle);
}

std::string watch_d::name() const noexcept {
    return "watch (" + file + ")";
}

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

file_d::~file_d() {
    close(handle);
}

event::event() {
    handle = epoll_create1(0);
    if (handle < 1)
        throw std::runtime_error(std::string("failed to create epoll: ") + std::strerror(errno));
}

void event::add(descriptor & f, std::uint32_t events, const std::function<void(descriptor &, std::uint32_t)> & action) {
    record_t * record = new record_t(f, action, events);
    data.emplace_back(record);
    if (epoll_ctl(handle, EPOLL_CTL_ADD, f.handle, &record->ud)) {
        throw std::runtime_error(std::string("failed to add file descriptor to epoll: ") + std::strerror(errno));
    }
}

void event::remove(descriptor & f) {
    for (auto iter = data.begin(), end = data.end(); iter != end; iter++) {
        if ((*iter)->d == f) {
            record_t * record = *iter;
            data.erase(iter);
            delete record;
            if (epoll_ctl(handle, EPOLL_CTL_DEL, f.handle, nullptr))
                throw std::runtime_error(std::string("failed to remove file descriptor from epoll: ") + std::strerror(errno));
        }
    }
}

std::string event::name() const noexcept {
    return "event poll";
}

std::vector<std::reference_wrapper<descriptor>> event::read(int timeout) {
    static const int max_event_n = 5;
    epoll_event events[max_event_n];
    int catched = epoll_wait(handle, events, max_event_n, timeout);
    if (catched < 0)
        throw std::runtime_error(std::string("failed to catch events from event pull: ") + std::strerror(errno));
    std::vector<std::reference_wrapper<descriptor>> result;
    for (int i = 0; i < catched; i++) {
        auto event = &events[i];
        record_t * record = reinterpret_cast<record_t *>(event->data.ptr);
        record->action(record->d, event->events);
        result.push_back(record->d);
    }
    return result;
}

void event::default_action(descriptor & f, std::uint32_t events) {
    log->debug("Cought event: " + f.name());
}

const std::function<void(descriptor &, std::uint32_t events)> event::default_action_p = &event::default_action;

event::~event() {
    for (auto ptr : data)
        delete ptr;
    close(handle);
}

}
