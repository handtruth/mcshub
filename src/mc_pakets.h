
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

	struct handshake final : public paket<ids::handshake, fields::varint, fields::string, fields::ushort, fields::varint> {
		constexpr std::int32_t & version() { return field<0>(); }
		constexpr const std::int32_t & version() const { return field<0>(); }
		constexpr std::string & address() { return field<1>(); }
		constexpr const std::string & address() const { return field<1>(); }
		constexpr std::uint16_t & port() { return field<2>(); }
		constexpr const std::uint16_t & port() const { return field<2>(); }
		constexpr std::int32_t & state() { return field<3>(); }
		constexpr const std::int32_t & state() const { return field<3>(); }

		// get value by property name
		std::string operator[](const std::string & name) const;

		static constexpr const char * name = "hs";
	};

	struct request final : public paket<ids::request> {};

	struct response final : public paket<ids::response, fields::string> {
		constexpr std::string & message() { return field<0>(); }
	};

	struct pinpong final : public paket<ids::pingpong, fields::long_f> {
		constexpr std::int64_t & payload() { return field<0>(); }
	};

	struct disconnect final : public paket<ids::disconnect, fields::string> {
		constexpr std::string & message() { return field<0>(); }
	};

	struct login final : public paket<ids::login, fields::string> {
		constexpr std::string & name() { return field<0>(); }
	};

}
}

#endif // !_MC_PAKETS_HEAD
