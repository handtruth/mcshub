#ifndef _STDIN_D_HEAD
#define _STDIN_D_HEAD

#include <istream>

#include "primitives.h"
#include "in_stream_d.h"

namespace mcshub {

class stdin_d final : public in_stream_d {
public:
	stdin_d() {
		handle = 0;
	}
	virtual std::string to_string() const noexcept override;
};

extern stdin_d input;

}

#endif // _STDIN_D_HEAD
