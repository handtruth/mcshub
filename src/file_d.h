#ifndef _FILE_D_HEAD
#define _FILE_D_HEAD

#include <fcntl.h>

#include "primitives.h"
#include "descriptor.h"

namespace mcshub {

class file_d : public descriptor {
private:
	std::string file;
public:
	enum class mode : int {
		ro = O_RDONLY,
		wo = O_WRONLY,
		wr = O_RDWR,
		aw = O_APPEND,
	};
	file_d(const std::string & path, mode m);
	virtual std::string to_string() const noexcept override;
	int read(byte_t bytes[], size_t length);
	int write(const byte_t bytes[], size_t length);
	virtual ~file_d() override;
};

} // namespace mcshub

#endif // _FILE_D_HEAD
