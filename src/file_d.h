#ifndef _FILE_D_HEAD
#define _FILE_D_HEAD

#include <fcntl.h>

#include "primitives.h"
#include "in_stream_d.h"
#include "out_stream_d.h"

namespace mcshub {

class file_d : public in_stream_d, public out_stream_d {
private:
	std::string file;
public:
	enum class mode : int {
		ro = O_RDONLY,
		wo = O_WRONLY,
		rw = O_RDWR,
		aw = O_APPEND,
	};
	file_d(const std::string & path, mode m);
	virtual std::string to_string() const noexcept override;
};

} // namespace mcshub

#endif // _FILE_D_HEAD
