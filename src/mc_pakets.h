
#ifndef _MC_PAKETS_HEAD
#define _MC_PAKETS_HEAD

#include "paket.h"

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

	struct handshake : public paket<ids::handshake, fields::varint, fields::string, fields::ushort, fields::varint> {
		constexpr std::int32_t & version() { field<0>(); }
		constexpr std::string & address() { field<1>(); }
		constexpr std::uint16_t & port() { field<2>(); }
		constexpr std::int32_t & state() { field<3>(); }
	};

	struct request : public paket<ids::request> {};

	struct response : public paket<ids::response, fields::string> {
		constexpr std::string & message() { field<0>(); }
	};

	struct pinpong : public paket<ids::pingpong, fields::long_f> {
		constexpr std::int64_t & payload() { field<0>(); }
	};

	struct disconnect : public paket<ids::disconnect, fields::string> {
		constexpr std::string & message() { field<0>(); }
	};

	struct login : public paket<ids::login, fields::string> {
		constexpr std::string & name() { field<0>(); }
	};

}
}

#endif // !_MC_PAKETS_HEAD
