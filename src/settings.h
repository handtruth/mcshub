#ifndef _SETTINGS_HEAD
#define _SETTINGS_HEAD

#include <cinttypes>
#include <unordered_map>
#include <exception>
#include <memory>
#include <vector>

#include "log.h"
#include "event_pull.h"
#include "dns_server.h"
#include "primitives.h"
#include "mutex_atomic.h"
#include "property.h"

namespace mcshub {

struct config {
	p<std::string> address;
	p<std::uint16_t> port;
	p<unsigned int> threads;

	p<std::string> log;
	p<log_level> verb;

	bool distributed;

	struct dns_module {
		bool use_dns;
		std::uint32_t ttl;
		std::vector<std::shared_ptr<dns_packet::answer_t::rdata_t>> records;
	} dns;

	struct server_record {
		std::string address;
		std::uint16_t port;

		std::string status;
		std::string login;

		p<std::string> log;
		bool allowFML;

		std::unordered_map<std::string, std::string> vars;
		std::vector<std::shared_ptr<dns_packet::answer_t::rdata_t>> dns;
	} default_server;

	std::unordered_map<std::string, server_record> servers;

	static void initialize();
	static void init_listener(event & poll);
	void load(const std::string & path);
	void install() const;
	static void static_install();
};

extern const matomic<std::shared_ptr<const config>> & conf;

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
