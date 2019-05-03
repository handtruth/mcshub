#ifndef _PUTIL_HEAD
#define _PUTIL_HEAD

namespace mcshub {

template <typename F>
class finalizator {
	F block;
public:
	finalizator(const finalizator & other) = delete;
	finalizator(finalizator && other) = delete;
	constexpr finalizator (F lambda) : block(lambda) {}
	~finalizator() {
		block();
	}
};

#define __mcshub_finnaly_internal(line, block) \
		finalizator __fblock##line([&]() -> void block )
#define finnaly(block) \
		__mcshub_finnaly_internal(__LINE__, block)

} // mcshub

#endif // _PUTIL_HEAD
