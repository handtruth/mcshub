#ifndef _IN_STREAM_D_HEAD
#define _IN_STREAM_D_HEAD

#include "in_stream.h"
#include "descriptor.h"

namespace mcshub {

class in_stream_d : public in_stream, virtual public descriptor {
public:
	virtual int read(byte_t data[], size_t length) override;
};

} // namespace mcshub

#endif // _IN_STREAM_D_HEAD
