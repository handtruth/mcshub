#ifndef _MANAGER_HEAD
#define _MANAGER_HEAD

#include <ekutils/cli.hpp>
#include <ekutils/stdin_d.hpp>
#include <ekutils/reader.hpp>

namespace mcshub {

class manager {
	ekutils::choice_cli_node root;
	ekutils::reader input = ekutils::reader(ekutils::input);
public:
	manager();
	void on_line();
};

} // namespace mcshub

#endif // _MANAGER_HEAD
