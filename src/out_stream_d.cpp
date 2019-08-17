#include "out_stream_d.h"

#include <system_error>
#include <cerrno>
#include <unistd.h>

namespace mcshub {

int out_stream_d::write(const byte_t data[], size_t length) {
	int s = ::write(handle, data, length);
    if (s == -1) {
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to write to " + to_string());
    }
	return s;
}

} // namespace mcshub
