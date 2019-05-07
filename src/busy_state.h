#ifndef _BUSY_STATE_HEAD
#define _BUSY_STATE_HEAD

#include <atomic>

namespace mcshub {

class busy_state {
    std::atomic_bool & state;
public:
    busy_state(std::atomic_bool & s) : state(s) {
        state = true;
    }
    busy_state(const busy_state & other) = delete;
    busy_state(busy_state && other) = delete;
    ~busy_state() {
        state = false;
    }
};

}

#endif // _BUSY_STATE_HEAD
