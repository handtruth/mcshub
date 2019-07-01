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

file_d::~file_d() {}

} // namespace mcshub
