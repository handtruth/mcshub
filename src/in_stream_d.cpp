#include "in_stream_d.h"

#include <system_error>
#include <cerrno>
#include <unistd.h>

namespace mcshub {

int in_stream_d::read(byte_t data[], size_t length) {
	int r = ::read(handle, data, length);
    if (r == -1 && errno != EWOULDBLOCK) {
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to read from " + to_string());
    }
	return r;
}

} // namespace mcshub
