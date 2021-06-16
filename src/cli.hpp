#ifndef CLI_HEAD_EFETPCP
#define CLI_HEAD_EFETPCP

#include <ekutils/cli.hpp>
#include <ekutils/stdin_d.hpp>
#include <ekutils/reader.hpp>

namespace mcshub {

class cli {
	ekutils::choice_cli_node root;
	ekutils::reader input = ekutils::reader(ekutils::input);
public:
	cli();
	void on_line();
};

} // namespace mcshub

#endif // CLI_HEAD_EFETPCP
