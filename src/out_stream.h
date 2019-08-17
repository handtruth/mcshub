#ifndef _OUT_STREAM_HEAD
#define _OUT_STREAM_HEAD

#include "primitives.h"

namespace mcshub {

struct out_stream {
    virtual int write(const byte_t data[], size_t length) = 0;
};

} // namespace mcshub

#endif // _OUT_STREAM_HEAD
