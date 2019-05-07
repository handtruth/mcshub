#include "test.h"
#include "event_pull.h"
#include "inotify_d.h"

#include <filesystem>

mcshub::inotify_d fs_event;

int main() {
    using namespace mcshub;
    namespace fs = std::filesystem;
    mcshub::event pull;
    fs_event.add_watch(inev::create, ".");
    pull.add(fs_event, actions::epoll_in, [](mcshub::descriptor & f, std::uint32_t) {
        assert_equals(fs_event.name(), f.name());
        inotify_d & inotify = dynamic_cast<inotify_d &>(f);
        auto whats = inotify.read();
        assert_equals(1, whats.size());
        assert_equals(fs::canonical("."), whats[0].watch.path());
        assert_equals(inev::create, inev::create & whats[0].mask);
    });
    fs::create_directory("inotify.d");
    auto ds = pull.pull(1000);
    fs::remove_all("inotify.d");
    assert_equals(1, ds.size());
    return 0;
}
