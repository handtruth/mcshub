#include "file_d.h"

#include <system_error>
#include <cerrno>
#include <unistd.h>

namespace mcshub {

file_d::file_d(const std::string & path, file_d::mode m) : file(path) {
	handle = open(path.c_str(), int(m));
	if (handle < 0)
		throw std::system_error(std::make_error_code(std::errc(errno)),
			"failed to open file \"" + path + "\"");
}

std::string file_d::to_string() const noexcept {
	return "file (" + file + ")";
}

} // namespace mcshub
