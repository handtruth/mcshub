#include "reallobuf.h"

#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace mcshub {

reallobuf::reallobuf(size_t initial) {
	bytes = (sz = initial) ? reinterpret_cast<byte_t *>(std::malloc(initial)) : nullptr;
}

reallobuf::reallobuf(const reallobuf & other) {
	if (sz = other.sz) {
		bytes = reinterpret_cast<byte_t *>(std::malloc(sz));
		std::memcpy(bytes, other.bytes, sz);
	} else
		bytes = nullptr;
}

reallobuf::reallobuf(reallobuf && other) {
	bytes = other.bytes;
	sz = other.sz;
	other.bytes = nullptr;
	other.sz = 0;
}

size_t reallobuf::probe(size_t amount) {
	if (amount > sz)
		bytes = reinterpret_cast<byte_t *>(std::realloc(bytes, sz = amount*k));
	return sz;
}

void reallobuf::move(size_t i) {
#ifdef DEBUG
	if (i > sz)
		throw std::range_error("argument i = " + std::to_string(i) + " and size = " + std::to_string(sz));
#endif
	std::memmove(bytes, bytes + i, sz - i);
}

reallobuf::~reallobuf() {
	free(bytes);
}

}
