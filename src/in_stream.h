#ifndef _IN_STREAM_HEAD
#define _IN_STREAM_HEAD

#include "primitives.h"

namespace mcshub {

struct in_stream {
	virtual int read(byte_t data[], size_t length) = 0;
};

} // namespace mcshub

#endif // _IN_STREAM_HEAD
