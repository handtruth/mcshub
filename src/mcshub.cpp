#include "mcshub.hpp"

#include <iostream>

#include <ekutils/signal_d.hpp>

#include "thread_controller.hpp"
#include "cli.hpp"
#include "mcshub_arguments.hpp"
#include "settings.hpp"
#include "config.hpp"

namespace mcshub {

int entrypoint_internal(const char * exe, ekutils::delegate_base<void(void)> * after_start) {
	if (arguments.help) {
		std::cout << arguments.build_help(exe);
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
	cli commands;
	ekutils::input.set_non_block();
	if (arguments.cli)
		poll.add(ekutils::input, [&commands](auto &, auto) {
			commands.on_line();
		});
	if (after_start) {
		(*after_start)();
	}
	while (true) poll.wait(-1);
	return EXIT_SUCCESS;
}

} // namespace mcshub
