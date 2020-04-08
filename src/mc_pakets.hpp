
#ifndef _MC_PAKETS_HEAD
#define _MC_PAKETS_HEAD

#define PAKET_LIB_EXT

#include <paket.hpp>

namespace mcshub {
namespace pakets {

	namespace ids {
		enum {
			handshake = 0,
			request = 0,
			response = 0,
			pingpong = 1,
			disconnect = 0,
			login = 0,
		};
	}

	using namespace handtruth::pakets;

	struct handshake final : public paket<ids::handshake, fields::varint, fields::string, fields::uint16, fields::varint> {
		fname(version, 0)
		fname(address, 1)
		fname(port, 2)
		fname(state, 3)

		// get value by property name
		std::string operator[](const std::string & name) const;

		static constexpr const char * name = "hs";
	};

	struct request final : public paket<ids::request> {};

	struct response final : public paket<ids::response, fields::string> {
		fname(message, 0)
	};

	struct pinpong final : public paket<ids::pingpong, fields::int64> {
		fname(payload, 0)
	};

	struct disconnect final : public paket<ids::disconnect, fields::string> {
		fname(message, 0)
	};

	struct login final : public paket<ids::login, fields::string> {
		fname(name, 0)
	};

}
}

#endif // _MC_PAKETS_HEAD
