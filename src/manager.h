#ifndef _MANAGER_HEAD
#define _MANAGER_HEAD

#include "mcshubcli.h"
#include "stdin_d.h"
#include "reader.h"

namespace mcshub {

class manager {
	choice_cli_node root;
	reader input = reader(mcshub::input);
public:
	manager();
	void on_line();
};

} // namespace mcshub

#endif // _MANAGER_HEAD
