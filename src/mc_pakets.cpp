#include "mc_pakets.h"

int test() {
	using namespace mcshub;
	pakets::handshake hs;
	hs.address() = "fedefgthgj";
	const mcshub::size_t sz = 255;
	byte_t b[sz];
	int s = hs.write(b, sz);
	std::to_string(hs);
	return s;
}
