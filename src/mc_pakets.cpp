#include "mc_pakets.h"

namespace mcshub {

std::string pakets::handshake::operator[](const std::string & name) const {
	if (name == "version")
		return std::to_string(version());
	else if (name == "address")
		return address();
	else if (name == "port")
		return std::to_string(port());
	else if (name == "state")
		return std::to_string(state());
	else
		return "{ NULL }";
}

}
