#include <string>
#include <cinttypes>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <chrono>

#include <netdb.h>

#include <ekutils/socket_d.hpp>
#include <ekutils/arguments.hpp>
#include <ekutils/expandbuff.hpp>

#include "config.hpp"
#include "sclient.hpp"

struct pargs_t : public ekutils::arguments {
	bool & help = add<flag>("help", [](flag & opt) {
		opt.hint = "display this help message and exit";
		opt.c = 'h';
	});
	bool & version = add<flag>("version", [](flag & opt) {
		opt.hint = "print version number and exit";
		opt.c = 'v';
	});
	bool & ping = add<flag>("ping", [](flag & opt) {
		opt.hint = "ping 3 times, after receiving status";
		opt.c = 'p';
	});
	pargs_t() {
		positional_hint = "<address> [record_name]";
	}
} pargs;

void print_usage(std::ostream & output, const std::string_view & prog) {
	output << pargs.build_help(prog) << R"==(
Arguments:
  address      address of a Minecraft server
               tcp://mc.example.com:25577
  record_name  virtual server name (default:
               addressed hostname)
)==";
}

int main(int argc, char *argv[]) {
	try {
		pargs.parse(argc, argv);
		if (!pargs.help && !pargs.version && pargs.positional.size() != 1 && pargs.positional.size() != 2)
			throw ekutils::arguments_parse_error("invalid number of positional arguments");
	} catch (const ekutils::arguments_parse_error & e) {
		std::cerr << e.what() << std::endl;
		print_usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	} catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	if (pargs.help) {
		print_usage(std::cout, argv[0]);
		return EXIT_SUCCESS;
	}
	if (pargs.version) {
		std::cout << config::build << std::endl;
		return EXIT_SUCCESS;
	}
	try {
		using namespace mcshub;
		const ekutils::uri uri = pargs.positional.front();
		sclient client(uri);
		const std::string host = (pargs.positional.size() == 2) ? std::move(pargs.positional[1]) : uri.get_host();
		const std::uint16_t port = (uri.get_port() == -1) ? 25565u : static_cast<std::uint16_t>(uri.get_port());
		pakets::response res = client.status(host, port);
		std::cout << res.message() << std::endl;
		using namespace std::chrono;
		if (pargs.ping) {
			for (int i = 0; i < 3; ++i) {
				std::chrono::milliseconds time = client.ping();
				std::cout << time.count() << "ms" << std::endl;
			}
		}
		return EXIT_SUCCESS;
	} catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
