#include "settings.h"
#include "log.h"
#include <iostream>
#include "socket_d.h"
#include "dns_server.h"
#include "server.h"
#include "prog_args.h"

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
	tcp_listener_d listener(c->address, c->port);
	log_verbose("start server on " + listener.name());
	poll.add(listener, [&listener, &poll](descriptor & fd, std::uint32_t events) {
		client * c = new client(listener.accept(), poll);
		log_verbose("new client: " + std::string(c->socket().remote_endpoint()));
		poll.add(c->socket(), actions::epoll_in | actions::epoll_rdhup, [&poll, c](descriptor & fd, std::uint32_t events) {
			if (!((*c)(events))) {
				log_verbose("dissconnected: " + std::string(c->socket().remote_endpoint()));
				delete c;
			}
		});
	});
	listener.start();
	/*
	udp_server_d dns("0.0.0.0", 5000);
	poll.add(dns, actions::epoll_in, [](descriptor & fd, std::uint32_t events){
		udp_server_d & dns = dynamic_cast<udp_server_d &>(fd);
		dns_packet packet;
		byte_t buffer[512];
		endpoint_info endpoint;
		dns.read(buffer, 512, &endpoint);
		packet.read(buffer, 512);
		std::string name = packet.questions.at(0).name;
		log_info("DNS requests " + std::to_string(packet.questions.size()) + " from " + std::string(endpoint) + ": " + name);
		packet.header.is_response = true;
		packet.header.is_trancated = false;
		packet.header.is_authoritative = false;
		packet.header.op_code = (std::uint16_t)dns_packet::header_t::rcode_t::no_error;
		packet.header.ar_count = 0;
		packet.answers.clear();
		packet.answer_A(name, 127 << 24 | 1, 4);
		int s = packet.write(buffer, 512);
		dns.write(buffer, s, endpoint);
	});
	*/
	c.reset();
	while (true) poll.pull(-1);
	return 0;
}
