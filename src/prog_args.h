#ifndef _PROG_ARGS_HEAD
#define _PROG_ARGS_HEAD

#include <string>
#include <cinttypes>
#include <vector>

namespace mcshub {

class arguments_t final {
public:
	std::string confname = "mcshub.yml";
	std::string status = "status.json";
	std::string login = "login.json";
	std::string default_srv_dir = "default";
	std::string domain = "";
	bool mcsman = false;
	bool install = false;
	std::uint16_t port = 25565;
	std::uint16_t default_port = 25565;

	std::vector<std::string> other;

	void parse(int argn, const char ** args);
} extern arguments;

} // mcshub

#endif // _PROG_ARGS_HEAD
