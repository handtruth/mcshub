#ifndef _STDIN_D_HEAD
#define _STDIN_D_HEAD

#include <istream>

#include "descriptor.h"

namespace mcshub {

class stdin_stream_buffer : public std::streambuf {
	int underflow() {
        return this->gptr() == this->egptr()
             ? std::char_traits<char>::eof()
             : std::char_traits<char>::to_int_type(*this->gptr());
    }
};

extern class stdin_d final : public descriptor {
public:
	stdin_d() {
		handle = 0;
	}
	virtual std::string to_string() const noexcept override;

} input;

}

#endif // _STDIN_D_HEAD
