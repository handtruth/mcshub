#ifndef _OUT_STREAM_D_HEAD
#define _OUT_STREAM_D_HEAD

#include "out_stream.h"
#include "descriptor.h"

namespace mcshub {

class out_stream_d : public out_stream, virtual public descriptor {
public:
	virtual int write(const byte_t data[], size_t length) override;
};

} // namespace mcshub

#endif // _OUT_STREAM_D_HEAD
