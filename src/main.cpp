#include <iostream>

#include "settings.h"
#include "log.h"
#include "prog_args.h"
#include "thread_controller.h"
#include "signal_d.h"
#include "event_pull.h"

const std::string version = "v1.1.0";

int main(int argc, char *argv[]) {
	using namespace mcshub;
	try {
		arguments.parse(argc - 1, (const char **)argv + 1);
	} catch (const std::exception & e) {
		std::cerr << "invalid program arguments: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	if (arguments.help) {
		std::cout << "Usage: " << argv[0] << R"""( [options]
Options:
  -h, --help                       display this help message and exit
  -v, --version                    print version number and exit
      --confname CONFIG_NAME       change configuration file name (mcshub.yml default)
      --status STATUS_FILE         change default status file name (status.json
	                               default)
      --login LOGIN_FILE           change default login file name (login.json default)
  -i, --install                    install the default MCSHub configuration
      --default-dir DEFAULT_DIR    specify name of a directory for the default entry
                                   ("default" is the default)
      --domian DOMAIN              specify domain name (the same option as in
                                   mcshub.yml file). This option will be overriden
                                   by "domain"field in mcshub.yml file
  -m, --mcsman                     enable extra functionality for MCSMan
      --port TCP_PORT              set listen port for MCSHub. This option will be
                                   overriden by port number in configuration file.
      --default-port DEFAULT_PORT  set the default TCP port number for all server
                                   records in configuration file.
)""";
		return EXIT_SUCCESS;
	}
	if (arguments.version) {
		std::cout << version << std::endl;
		return EXIT_SUCCESS;
	}
	if (arguments.install) {
		config::static_install();
		return EXIT_SUCCESS;
	}
	stdout_log l(log_level::debug);
	log = &l;
	config::initialize();
	log_info("switch log depends configuration");
	std::shared_ptr<const config> c = conf;
	if ((const std::string &)(c->log) == "$std")
		log->set_log_level(c->verb);
	else
		log = new file_log(c->log, c->verb);
	signal_d signal { sig::abort, sig::broken_pipe, sig::termination, sig::segmentation_fail };
	event poll;
	config::init_listener(poll);
	log_verbose("current version -- " + version);
	log_verbose("start server on " + c->address + ':' + std::to_string(c->port));
	c.reset();
	thread_controller controller;
	poll.add(signal, [&signal, &controller](descriptor &, std::uint32_t) {
		switch (signal.read()) {
			case sig::abort:
				log_fatal("abort signal received, a part of the MCSHub was destroyed");
				return;
			case sig::segmentation_fail:
				// Very bad...
				log_fatal("segmentation fail");
				return;
			case sig::broken_pipe:
				return;
			case sig::termination:
				log_info("terminating MCSHub instance...");
				controller.terminate();
				log_info("successfuly stoped MCSHub");
				std::exit(EXIT_SUCCESS);
				return;
			default:
				return;
		}
	});
	while (true) poll.pull(-1);
	return EXIT_SUCCESS;
}
