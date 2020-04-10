#ifndef _MCSHUB_HEAD
#define _MCSHUB_HEAD

#include <ekutils/delegate.hpp>

namespace mcshub {

int entrypoint_internal(const char * exe, ekutils::delegate_base<void(void)> * after_start);

inline int entrypoint(const char * exe) {
    return entrypoint_internal(exe, nullptr);
}

template <typename F>
inline int entrypoint(const char * exe, F after_start) {
    auto delegate = ekutils::delegate_t<F, void(void)>(after_start);
    return entrypoint_internal(exe, &delegate);
}

} // namespace mcshub

#endif // _MCSHUB_HEAD
