#ifndef _PROG_ARGS_HEAD
#define _PROG_ARGS_HEAD

#include <string>
#include <cinttypes>
#include <vector>

#include <ekutils/log.hpp>
#include <ekutils/arguments.hpp>

namespace mcshub {

struct arguments_t final : public ekutils::arguments {
	std::string & confname;
	std::string & status;
	std::string & login;
	std::string & default_srv_dir;
	std::string & domain;
	bool & mcsman;
	bool & install;
	bool & distributed;
	bool & drop;
	std::string & address;
	std::intmax_t & default_port;
	std::intmax_t & max_packet_size;
	std::intmax_t & timeout;
	bool & version;
	bool & help;
	bool & cli;
	bool & no_dns_cache;
	std::intmax_t & threads;
	ekutils::log_level & verb;
	multistring::value_type & control;

private:
	void copy_from(const arguments_t & other) {
		confname = other.confname;
		status = other.status;
		login = other.login;
		default_srv_dir = other.default_srv_dir;
		domain = other.domain;
		mcsman = other.mcsman;
		install = other.install;
		distributed = other.distributed;
		drop = other.drop;
		address = other.address;
		default_port = other.default_port;
		max_packet_size = other.max_packet_size;
		timeout = other.timeout;
		version = other.version;
		help = other.help;
		cli = other.cli;
		no_dns_cache = other.no_dns_cache;
		threads = other.threads;
		verb = other.verb;
	}

public:
	arguments_t();
	arguments_t(const arguments_t & other) : arguments_t() {
		copy_from(other);
	}
	arguments_t & operator=(const arguments_t & other) {
		copy_from(other);
		return *this;
	}
};

extern arguments_t arguments;

} // mcshub

#endif // _PROG_ARGS_HEAD
