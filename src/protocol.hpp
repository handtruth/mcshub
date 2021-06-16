#ifndef PROTOCOL_HEAD_PAOCOTKSAALKD
#define PROTOCOL_HEAD_PAOCOTKSAALKD

#define PAKET_LIB_EXT
#include <paket.hpp>

namespace mcshub {
namespace protocol {

using namespace handtruth::pakets;

namespace ids {
	enum : unsigned {
		handshake,
		error,
		record,
		get_record,
		no_record,
		drop_record,
	};
}

constexpr unsigned version = 1u;

struct handshake : public paket<ids::handshake, fields::zint<unsigned>, fields::string, fields::uint16> {
	fname(version, 0)
	fname(address, 1)
	fname(port, 2)

	handshake() {
		version() = protocol::version;
	}
};

struct error : public paket<ids::error, fields::zint<unsigned>, fields::zint<unsigned>, fields::string> {
	enum class error_codes {
		success, unknown
	};

	ename(last, 0, decltype(ids::handshake))
	ename(error_code, 1, error_codes)
	fname(message, 2)
};

template <std::int32_t paket_id, typename ...fields_t>
struct record_head : public paket<paket_id, fields::string, fields::boolean, fields_t...> {
	fname(name, 0)
	fname(fml, 1)
};

struct record : public record_head<
	ids::record, fields::string, fields::uint16, fields::string, fields::string,
	fields::list<fields::string>, fields::list<fields::string>
> {
	fname(address, 2)
	fname(port, 3)
	fname(status, 4)
	fname(login, 5)
	lname(vars_keys, 6)
	lname(vars_values, 7)
};

struct get_record : public record_head<ids::get_record> {};

struct no_record : public record_head<ids::no_record> {};

struct drop_record : public paket<ids::drop_record> {};

} // namespace protocol
} // namespace mcshub

#endif // PROTOCOL_HEAD_PAOCOTKSAALKD
