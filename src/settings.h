#ifndef _SETTINGS_HEAD
#define _SETTINGS_HEAD

#include <cinttypes>
#include <vector>
#include <unordered_map>
#include <istream>
#include <exception>

#include "log.h"

namespace mcshub {

struct config {
	std::string address;
	std::uint16_t port;
	unsigned int threads;

	std::string log;
	log_level verb;

	struct server_record {
		std::string address;
		std::uint16_t port;

		std::string status;
		std::string login;

		std::string log;

		std::unordered_map<std::string, std::string> vars;
	} default_server;

	std::unordered_map<std::string, server_record> servers;

	void load(const std::string & path);
	void install();

} extern conf;

class config_exception : public std::exception {
private:
	std::string cause;
public:
	config_exception(const char * field, const std::string & message) noexcept :
		cause(std::string("field \"") + field + "\" has incorrect value (" + message + ')') {}

	virtual const char * what() const noexcept override {
		return cause.c_str();
	}
};

} // mcshub

#endif // _SETTINGS_HEAD
