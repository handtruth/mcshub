#ifndef VARS_BASE_HEAD_QPVUUUJWE
#define VARS_BASE_HEAD_QPVUUUJWE

#include <string>
#include <optional>

namespace mcshub {

struct vars_base {
	virtual std::optional<std::string> get(const std::string & ns, const std::string & var) const = 0;
	virtual ~vars_base();
};

struct empty_vars : public vars_base {
	virtual std::optional<std::string> get(const std::string &, const std::string &) const override {
		return {};
	}
};

} // namespace mcshub

#endif // VARS_BASE_HEAD_QPVUUUJWE
