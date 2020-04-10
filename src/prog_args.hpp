#ifndef _PROG_ARGS_HEAD
#define _PROG_ARGS_HEAD

#include <string>
#include <cinttypes>
#include <vector>

#include <ekutils/log.hpp>

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
	bool distributed = false;
	bool drop = false;
	std::string address = "0.0.0.0";
	std::uint16_t port = 25565;
	std::uint16_t default_port = 25565;
	int max_packet_size = 6000;
	unsigned long timeout = 500;
	bool version = false;
	bool help = false;
	bool cli = false;
	bool no_dns_cache = false;
	unsigned threads = std::numeric_limits<unsigned>::max();
	ekutils::log_level verb = ekutils::log_level::info;

	std::vector<std::string> other;

	void parse(int argn, const char ** args);
};

extern arguments_t arguments;

} // mcshub

#endif // _PROG_ARGS_HEAD
