#include <iostream>

#include "settings.h"
#include "log.h"
#include "prog_args.h"
#include "thread_controller.h"
#include "signal_d.h"
#include "event_pull.h"

int main(int argc, char *argv[]) {
	using namespace mcshub;
	arguments.parse(argc, (const char **)argv);
	if (arguments.install) {
		config::static_install();
		return 0;
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
				std::terminate();
				return;
			default:
				return;
		}
	});
	while (true) poll.pull(-1);
	return 0;
}
