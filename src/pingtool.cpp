#include <string>
#include <cinttypes>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <chrono>

#include <netdb.h>

#include <ekutils/socket_d.hpp>
#include <ekutils/expandbuff.hpp>

#include "config.hpp"
#include "sclient.hpp"

struct host_error : public std::runtime_error {
	explicit host_error(const std::string & message) : std::runtime_error(message) {}
};

struct host_info {
	std::string host;
	std::string port;
	host_info(const char * str) {
		switch (*str) {
			case '\0':
				return;
			case '[': {
				const char * begin = str;
				while (*str != ']') {
					if (*str == '\0')
						throw host_error("ipv6 square bracket '[' not closed");
					str++;
				}
				host.append(begin + 1, str - begin - 1);
				if (*str == ']')
					str++;
				break;
			}
			default: {
				const char * begin = str;
				while (*str != ':' && *str) str++;
				host.append(begin, str - begin);
				break;
			}
		}
		if (*str) {
			str++;
			port = str;
		} else {
			port = "25565";
		}
	}
};

void print_usage(std::ostream & output, const char * prog) {
	output << "Usage: " << prog << R"==( <option> | <address[:port] [action]>
Options:
  -h, --help     display this help message and exit
  -v, --version  print version number and exit
Actions:
  status         fetch JSON status object
  ping           ping server
)==";
}

enum class acts_enum {
	status, ping
};

void entry(host_info & host, acts_enum act);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "1 argument required at least" << std::endl;
		print_usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	if (argc == 2) {
		std::string opt = argv[1];
		if (opt == "-v" || opt == "--version") {
			std::cout << config::build << std::endl;
			return 0;
		} else if (opt == "-h" || opt == "--help") {
			print_usage(std::cout, argv[0]);
			return 0;
		}
	}
	if (argc >= 4) {
		std::cerr << "maximum 2 arguments are allowed" << std::endl;
		print_usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	acts_enum act = acts_enum::status;
	if (argc == 3) {
		std::string opt = argv[2];
		if (opt == "status")
			act = acts_enum::status;
		else if (opt == "ping")
			act = acts_enum::ping;
		else {
			std::cerr << "only status and ping actions supported" << std::endl;
			print_usage(std::cerr, argv[0]);
			return EXIT_FAILURE;
		}
	}
	try {
		host_info host(argv[1]);
		entry(host, act);
	} catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void entry(host_info & host, acts_enum act) {
	using namespace mcshub;
	sclient client(host.host, host.port);
	switch (act) {
		case acts_enum::status: {
			pakets::response res = client.status(host.host);
			std::cout << res.message() << std::endl;
			break;
		}
		case acts_enum::ping: {
			client.status(host.host);
			using namespace std::chrono;
			std::chrono::milliseconds time = client.ping();
			std::cout << time.count() << "ms" << std::endl;
			break;
		}
	}
}
