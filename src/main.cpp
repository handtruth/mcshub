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
	config::initialize();
	event poll;
	config::init_listener(poll);
	stdout_log l(log_level::debug);
	log = &l;
	tcp_listener_d listener("0.0.0.0", 25565);
	log->verbose("start server on " + listener.name());
	poll.add(listener, [&listener, &poll](descriptor & fd, std::uint32_t events) {
		client * c = new client(listener.accept(), poll);
		mcshub::log->verbose("new client: " + std::string(c->socket().remote_endpoint()));
		poll.add(c->socket(), actions::epoll_in | actions::epoll_rdhup, [&poll, c](descriptor & fd, std::uint32_t events) {
			if (!((*c)(events))) {
				mcshub::log->verbose("dissconnected: " + std::string(c->socket().remote_endpoint()));
				delete c;
			}
		});
	});
	listener.start();
	udp_server_d server("0.0.0.0", 25565);
	poll.add(server, actions::epoll_in, [](descriptor & fd, std::uint32_t events){
		udp_server_d & srv = dynamic_cast<udp_server_d &>(fd);
		log->debug("GOT UDP DGRAM!!!");
		byte_t buffer[256];
		endpoint_info endpoint;
		int n = srv.read(buffer, 256, &endpoint);
		log->info("Datagram from " + std::string(endpoint) + ": " + std::string(reinterpret_cast<const char *>(buffer), n));
		byte_t msg[] = "lol kek cheburek\n";
		srv.write(msg, sizeof(msg), endpoint);
	});
	udp_server_d dns("0.0.0.0", 5000);
	poll.add(dns, actions::epoll_in, [](descriptor & fd, std::uint32_t events){
		udp_server_d & dns = dynamic_cast<udp_server_d &>(fd);
		dns_packet packet;
		byte_t buffer[512];
		endpoint_info endpoint;
		dns.read(buffer, 512, &endpoint);
		packet.read(buffer, 512);
		std::string name = packet.questions.at(0).name;
		log->info("DNS requests " + std::to_string(packet.questions.size()) + " from " + std::string(endpoint) + ": " + name);
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
	while (true) poll.pull(-1);
	return 0;
}
