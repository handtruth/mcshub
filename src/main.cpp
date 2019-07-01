#include "settings.h"
#include "log.h"
#include <iostream>
#include "prog_args.h"
#include "thread_controller.h"

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
	c->verb.listen([](log_level oldv, log_level newv) {
		log->set_log_level(newv);
	});
	c->log.listen([&](const std::string & oldv, const std::string & newv) {
		if (oldv != newv) {
			if (newv == "$std") {
				l.set_log_level(log->log_lvl());
				delete log;
				log = &l;
			} else {
				log_level lvl = log->log_lvl();
				try {
					dynamic_cast<stdout_log *>(log);
					log = new file_log(newv, lvl);
				} catch (...) {
					delete log;
					log = new file_log(newv, lvl);
				}
			}
		}
	});
	event poll;
	config::init_listener(poll);
	log_verbose("start server on " + c->address + ':' + std::to_string(c->port));
	c.reset();
	thread_controller controller;
	while (true) poll.pull(-1);
	return 0;
}
