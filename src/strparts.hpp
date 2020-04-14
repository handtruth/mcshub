#ifndef STRPARTS_HEAD_WPGJSPVK
#define STRPARTS_HEAD_WPGJSPVK

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>

#include "vars_base.hpp"

namespace mcshub {

class strparts {
	std::string source;

public:
	struct address_t {
		std::string ns;
		std::string var;
	};
	struct segment_t {
		unsigned offset;
		unsigned size;
		std::string_view of(const std::string & base) const {
			return std::string_view(base.data() + offset, size);
		}
	};
	typedef std::variant<segment_t, address_t> part_t;

private:
	std::vector<part_t> parts;
	mutable std::vector<std::string> buffer;

public:
	strparts();
	explicit strparts(std::string && src);

	constexpr const std::string & src() const noexcept {
		return source;
	}

	bool empty() const noexcept {
		return source.empty() && parts.empty();
	}

	bool simple() const noexcept;

	void append(const strparts & other);
	void append_jstr(const strparts & other);
	void append(const std::string & string);
	void append(const std::string_view & string);
	void append(const char * string);
	void append(const std::string & ns, const std::string & var);
	void simplify();

	strparts resolve_partial(const vars_base & vars) const;
	std::string resolve(const vars_base & vars = empty_vars()) const;
};

} // namespace mcshub

#endif // STRPARTS_HEAD_WPGJSPVK
