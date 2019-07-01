#ifndef _EVENT_D_HEAD
#define _EVENT_D_HEAD

#include "descriptor.h"

namespace mcshub {

class event_d : public descriptor {
public:
	event_d(unsigned int initval = 0);
	std::uint64_t read();
	void write(std::uint64_t value);
	virtual std::string to_string() const noexcept override;
};

} // namespace mcshub


#endif // _EVENT_D_HEAD
