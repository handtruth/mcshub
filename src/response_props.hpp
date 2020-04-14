#ifndef _RESPONSE_PROPS_HEAD
#define _RESPONSE_PROPS_HEAD

#include <tuple>
#include <unordered_map>
#include <vector>
#include <variant>
#include <array>
#include <functional>
#include <string_view>

#include "vars_base.hpp"
#include "mc_pakets.hpp"

namespace mcshub {

struct file_vars final {
	static constexpr const char * name = "file";
	std::reference_wrapper<const std::string> srv_name;
	file_vars();
	std::string operator[](const std::string & name) const;
};

struct img_vars final {
	static constexpr const char * name = "img";
	std::reference_wrapper<const std::string> srv_name;
	img_vars();
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
class vars_manager final : public std::tuple<const vars_t &...>, public vars_base {
	template <typename first, typename ...others>
	static std::optional<std::string> find_in_next(const std::string & ns, const std::string & name, const first & var, const others &... vars) {
		if (first::name == ns)
			return var[name];
		else
			return find_in_next(ns, name, vars...);
	}
	template <typename first>
	static std::optional<std::string> find_in_next(const std::string & ns, const std::string & name, const first & var) {
		if (first::name == ns)
			return var[name];
		else
			return { /* nothing */ };
	}
public:
	explicit vars_manager(const vars_t &... vars) : std::tuple<const vars_t &...>(vars...) {}
	
	virtual std::optional<std::string> get(const std::string & ns, const std::string & var) const override {
		if (ns.empty())
			return std::get<0>((const std::tuple<const vars_t &...> &)*this)[var];
		auto find_value = [&ns, &var](auto const &... e) -> std::optional<std::string> {
			return find_in_next(ns, var, e...);
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
