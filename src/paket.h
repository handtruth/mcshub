#ifndef _PAKET_HEAD
#define _PAKET_HEAD

#include <cinttypes>
#include <tuple>
#include <string>
#include <exception>

#include "log.h"
#include "primitives.h"

namespace mcshub {

namespace fields {

	template <typename T>
	class field {
	public:
		typedef T value_type;
		T value;
		T & operator*() noexcept {
			return value;
		}
		const T & operator*() const noexcept {
			return value;
		}
		template <typename other_t>
		bool operator==(const other_t & other) const noexcept {
			return value == other.value;
		}
	};

	class varint : public field<std::int32_t> {
	public:
		size_t size() const noexcept;
		int read(const byte_t bytes[], size_t length);
		int write(byte_t bytes[], size_t length) const;
		operator std::string() const;
	};

	class string : public field<std::string> {
	public:
		size_t size() const noexcept;
		int read(const byte_t bytes[], size_t length);
		int write(byte_t bytes[], size_t length) const;
		operator std::string() const;
	};

	template <typename T>
	class static_size_field : public field<T> {
	private:
		constexpr size_t static_size() const noexcept {
			return sizeof(typename field<T>::value_type);
		}

	public:
		size_t size() const noexcept {
			return static_size();
		}
		int read(const byte_t bytes[], size_t length) {
			if (length < static_size())
				return -1;
			field<T>::value = *(reinterpret_cast<const typename field<T>::value_type *>(bytes));
			return static_size();
		}
		int write(byte_t bytes[], size_t length) const {
			if (length <= static_size())
				return -1;
			*(reinterpret_cast<typename field<T>::value_type *>(bytes)) = field<T>::value;
			return static_size();
		}
		operator std::string() const {
			return std::to_string(field<T>::value);
		}
	};

	class ushort : public static_size_field<uint16_t> {};
	class long_f : public static_size_field<int64_t> {};
}

size_t size_varint(std::int32_t value);
int read_varint(const byte_t bytes[], size_t length, std::int32_t & value);
int write_varint(byte_t bytes[], size_t length, std::int32_t value);

class paket_error : public std::runtime_error {
public:
	paket_error(const std::string & message) : std::runtime_error(message) {}
};

namespace pakets {

	int head(const byte_t bytes[], size_t length, std::int32_t & size, std::int32_t & id);

	template <int paket_id, typename ...fields_t>
	class paket : public std::tuple<fields_t...> {
	private:
		template <typename first, typename ...other>
		static size_t size_field(const first & field, const other &... fields) noexcept {
			return field.size() + size_field(fields...);
		}
		static constexpr size_t size_field() noexcept {
			return 0;
		}
	public:
		paket() {}
		size_t size() const {
			auto size_them = [](auto const &... e) -> size_t {
				return size_field(e...);
			};
			return std::apply(size_them, (const std::tuple<fields_t...> &) *this);
		}
		template <int i>
		constexpr auto field() noexcept -> decltype(std::get<i>(*this).value) & {
			return std::get<i>(*this).value;
		}
		template <int i>
		constexpr auto field() const noexcept -> const decltype(std::get<i>(*this).value) & {
			return std::get<i>(*this).value;
		}
		constexpr int id() const noexcept {
			return paket_id;
		}
	private:
		template <typename first, typename ...other>
		static int write_field(byte_t bytes[], size_t length, const first & field, const other &... fields) {
			int s = field.write(bytes, length);
			if (s < 0)
				return -1;
			int comp_size = write_field(bytes + s, length - s, fields...);
			if (comp_size < 0)
				return -1;
			return s + comp_size;
		}
		static int write_field(byte_t bytes[], size_t length) {
			return 0;
		}
		template <typename first, typename ...other>
		static int read_field(const byte_t bytes[], size_t length, first & field, other &... fields) {
			int s = field.read(bytes, length);
			if (s < 0)
				return -1;
			int comp_size = read_field(bytes + s, length - s, fields...);
			if (comp_size < 0)
				return -1;
			return s + comp_size;
		}
		static int read_field(const byte_t bytes[], size_t length) {
			return 0;
		}
	public:
		int write(byte_t bytes[], size_t length) const {
			// HEAD
			// size of size
			int k = write_varint(bytes, length, size_varint(paket_id) + size());
			if (k < 0)
				return -1;
			// size of id
			int s = write_varint(bytes + k, length - k, paket_id);
			if (s < 0)
				return -1;
			s += k;
			// BODY
			auto write_them = [bytes, length, s](auto const &... e) -> int {
				return write_field(bytes + s, length - s, e...);
			};
			int comp_size = std::apply(write_them, (const std::tuple<fields_t...> &) *this);
			if (comp_size < 0)
				return -1;
			else
				return comp_size + s;
		}
		int read(const byte_t bytes[], size_t length) {
			std::int32_t size;
			std::int32_t id;
			// HEAD
			int k = read_varint(bytes, length, size);
			if (k < 0)
				return -1;
			if ((size_t)(size + k) > length)
				return -1;
			int s = read_varint(bytes + k, length - k, id);
			if (s < 0)
				return -1;
			if (id != paket_id)
				throw paket_error("wrong paket id (" + std::to_string(paket_id) + " expected, got " + std::to_string(id) + ")");
			int l = k + s;
			// BODY
			auto read_them = [bytes, length, l](auto &... e) -> int {
				return read_field(bytes + l, length - l, e...);
			};
			int comp_size = std::apply(read_them, (std::tuple<fields_t...> &) *this);
			if (comp_size < 0)
				return -1;
			if (size != comp_size + s)
				throw paket_error("wrong paket size (" + std::to_string(size) + " expected, got " + std::to_string(comp_size + s) + ")");
			else
				return comp_size + l;
		}
	private:
		template <typename first, typename ...other>
		static std::string enum_next_as_string(const first & field, const other &... fields) {
			return std::string(field) + ((", " + std::string(fields)) + ...);
		}
		template <typename first>
		static std::string enum_next_as_string(const first & field) {
			return std::string(field);
		}

		static std::string enum_next_as_string() {
			return std::string();
		}

	public:
		std::string enumerate_as_string() const {
			auto enum_them = [](auto const &... e) -> std::string {
				return enum_next_as_string(e...);
			};
			return std::apply(enum_them, (const std::tuple<fields_t...> &) *this);
		}
	};

}

}

namespace std {

	template < int id, typename ...fields_t >
	string to_string(const mcshub::pakets::paket<id, fields_t...> & paket) {
		std::string s = paket.enumerate_as_string();
		return "#" + to_string(id) + ":{ " + s + " }";
	}

}

#endif // _PAKET_HEAD
