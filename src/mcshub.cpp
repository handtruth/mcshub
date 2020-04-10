#include "mcshub.hpp"

#include <iostream>

#include <ekutils/signal_d.hpp>

#include "thread_controller.hpp"
#include "manager.hpp"
#include "prog_args.hpp"
#include "settings.hpp"
#include "config.hpp"

namespace mcshub {

int entrypoint_internal(const char * exe, ekutils::delegate_base<void(void)> * after_start) {
	if (arguments.help) {
		std::cout << "Usage: " << exe << R"==( [options]
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
  -d, --distributed                enable distributed MCSHub configuration
      --drop                       drop if no server entry matches
      --max-packet-size            set maximum allowed packet size for Minecraft
                                   protocol
      --timeout                    set timeout
  -m, --mcsman                     enable extra functionality for MCSMan
      --port TCP_PORT              set listen port for MCSHub. This option will be
                                   overriden by port number in configuration file
      --default-port DEFAULT_PORT  set the default TCP port number for all server
                                   records in configuration file
      --no-dns-cache               disable dns cache by default. This option will be
                                   overriden by dns_cache option in configuration file
      --verb LOG_LEVEL             set default verbose level to LOG_LEVEL. This option
                                   will be overriden by 'verb' option in configuration
                                   file. ("info" is the default value)
  -t, --threads THREADS            number of working theads. (option 'threads' in
                                   configuration file, default value equals to number
                                   of parallel threads supported by CPU)
)==";
		return EXIT_SUCCESS;
	}
	if (arguments.version) {
		std::cout << config::build << std::endl;
		return EXIT_SUCCESS;
	}
	if (arguments.install) {
		settings::static_install();
		return EXIT_SUCCESS;
	}

	ekutils::stdout_log l(ekutils::log_level::debug);
	ekutils::log = &l;
	settings::initialize();
	log_info("switch log depends configuration");
	std::shared_ptr<const settings> c = conf;
	if ((const std::string &)(c->log) == "$std")
		ekutils::log->set_log_level(c->verb);
	else
		ekutils::log = new ekutils::file_log(c->log, c->verb);
	using ekutils::sig;
	ekutils::signal_d signal { sig::abort, sig::broken_pipe, sig::termination, sig::segmentation_fail };
	ekutils::epoll_d poll;
	settings::init_listener(poll);
	log_verbose("current version -- " + config::build);
	thread_controller controller;
	log_verbose("start server on " + c->address + ':' + std::to_string(thread_controller::real_port));
	c.reset();
	poll.add(signal, [&signal, &controller](auto &, std::uint32_t) {
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
	manager manager;
	ekutils::input.set_non_block();
	if (arguments.cli)
		poll.add(ekutils::input, [&manager](auto &, auto) {
			manager.on_line();
		});
	if (after_start) {
		(*after_start)();
	}
	while (true) poll.wait(-1);
	return EXIT_SUCCESS;
}

} // namespace mcshub
