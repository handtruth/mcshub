#ifndef _RESPONSE_PROPS_HEAD
#define _RESPONSE_PROPS_HEAD

#include <tuple>
#include <unordered_map>
#include <vector>
#include <variant>
#include <array>

#include <ekutils/parse_essentials.hpp>

#include "mc_pakets.hpp"

namespace mcshub {

struct file_vars final {
	static constexpr const char * name = "file";
	std::string srv_name = ".";
	std::string operator[](const std::string & name) const;
};

struct env_vars_t final {
	static constexpr const char * name = "env";
	std::string operator[](const std::string & name) const;
};
extern env_vars_t env_vars;

struct server_vars final {
	static constexpr const char * name = "var";
	const std::unordered_map<std::string, std::string> * vars;
	std::string operator[](const std::string & name) const;
};

struct main_vars_t final {
	static constexpr const char * name = "main";
	std::string operator[](const std::string & name) const;
};
extern main_vars_t main_vars;

template <typename ...vars_t>
class vars_manager final : public std::tuple<const vars_t &...> {
	struct string_cut {
		const char * chunk;
		unsigned size;
	};
	template <typename first, typename ...others>
	static std::string find_in_next(const std::string & ns, const std::string & name, const first & var, const others &... vars) {
		if (first::name == ns)
			return var[name];
		else
			return find_in_next(ns, name, vars...);
	}
	template <typename first>
	static std::string find_in_next(const std::string & ns, const std::string & name, const first & var) {
		if (first::name == ns)
			return var[name];
		else
			return "{ NO NSPA }";
	}
	static bool isvar(char c) {
		return std::isalnum(c) || c == ':' || c == '_';
	}
public:
	explicit vars_manager(const vars_t &... vars) : std::tuple<const vars_t &...>(vars...) {}

	inline std::string resolve(const std::string & content) const {
		return resolve(content.c_str(), content.size());
	}
	template <std::size_t N>
	std::string resolve(const std::array<std::uint8_t, N> & content) const {
		return resolve(reinterpret_cast<const char *>(content.data()), content.size());
	}
	std::string resolve(const char *str, std::size_t length) const {
		std::vector<std::variant<string_cut, std::string>> result;
		std::size_t i, j;
		for (i = 0, j = 0; i < length;) {
			if (str[i] == '$') {
				if (!isvar(str[i + 1]) && str[i + 1] != '{' && str[i + 1] != '$') {
					i += 1;
					continue;
				}
				result.emplace_back(string_cut { str + j, static_cast<unsigned>(i - j) });
				i++;
				switch (str[i]) {
					case '$': {
						result.emplace_back(string_cut { "$", 1 });
						i++;
						j = i;
						break;
					}
					case '{': {
						int t = ekutils::find_closing_bracket<'{'>(str + i);
						std::string value = get_value(str + i + 1, t - 2);
						result.push_back(std::move(value));
						i += t;
						j = i;
						break;
					}
					default: {
						std::size_t t;
						for (t = 0; isvar(str[i + t]); t++);
						std::string value = get_value(str + i, t);
						result.push_back(std::move(value));
						i += t;
						j = i;
					}
				}
			} else
				i++;
		}
		if (i != j)
			result.emplace_back(string_cut { str + j, static_cast<unsigned>(i - j) });
		std::size_t size = 0;
		for (auto & str : result) {
			if (std::holds_alternative<std::string>(str))
				size += std::get<std::string>(str).size();
			else
				size += std::get<string_cut>(str).size;
		}
		std::string result_string;
		result_string.reserve(size);
		for (auto & str : result) {
			if (std::holds_alternative<std::string>(str))
				result_string += std::get<std::string>(str);
			else {
				const string_cut & cut = std::get<string_cut>(str);
				result_string.append(cut.chunk, cut.size);
			}
		}
		return result_string;
	}

	std::string get_value(const char * str, std::size_t length) const {
		ekutils::trim_string(str, length);
		int d = ekutils::find_char<':'>(str, length);
		if (d < 0) {
			return std::get<0>((const std::tuple<const vars_t &...> &)*this)[std::string(str, length)];
		}
		std::size_t ns_len = d;
		ekutils::trim_string(str, ns_len);
		std::string ns = std::string(str, ns_len);
		const char * name_raw = str + d + 1;
		std::size_t name_raw_len = length - d - 1;
		ekutils::trim_string(name_raw, name_raw_len);
		std::string name = std::string(name_raw, name_raw_len);
		auto find_value = [&ns, &name](auto const &... e) -> std::string {
			return find_in_next(ns, name, e...);
		};
		return std::apply(find_value, (const std::tuple<const vars_t &...> &) *this);
	}
};

template <typename ...vars_t>
vars_manager<vars_t...> make_vars_manager(const vars_t &... vars) {
	return vars_manager<vars_t...>(vars...);
}

} // mcshub

#endif // _RESPONSE_PROPS_HEAD
