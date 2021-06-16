#include "thread_controller.hpp"

#include <stdexcept>
#include <sys/syscall.h>

#include <ekutils/resolver.hpp>

#include "settings.hpp"
#include "mcshub_arguments.hpp"

extern "C" long syscall(long number, ...);

namespace mcshub {

ekutils::net::port_t thread_controller::real_port = 0;

std::uint16_t thread_id() {
	thread_local std::uint16_t id = static_cast<std::uint16_t>(syscall(SYS_gettid));
	return id;
}

std::uint16_t getport(const ekutils::net::endpoint & endpoint) {
	switch (endpoint.family()) {
		case ekutils::net::family_t::ipv4:
			return dynamic_cast<const ekutils::net::ipv4::endpoint &>(endpoint).port();
		case ekutils::net::family_t::ipv6:
			return dynamic_cast<const ekutils::net::ipv6::endpoint &>(endpoint).port();
		default:
			return 0;
	}
}

worker::worker() : working(true) {
	conf_snap c;
	auto targets = ekutils::net::resolve(
		ekutils::net::socket_types::stream,
		c->address,
		static_cast<std::uint16_t>(arguments.default_port) | thread_controller::real_port
	);
	listener = ekutils::net::bind_stream_any(targets.begin(), targets.end(),
		ekutils::net::socket_flags::reuse_port | ekutils::net::socket_flags::non_block);
	listener->listen();
	if (!thread_controller::real_port) {
		thread_controller::real_port = getport(listener->local_endpoint());
	}
	multiplexor.add(*listener, [this](ekutils::descriptor & fd, std::uint32_t events) {
		on_accept(fd, events);
	});
	events.set_non_block();
	using namespace ekutils::actions;
	multiplexor.add(events, in | out | et, [this](ekutils::descriptor & fd, std::uint32_t events) {
		on_event(fd, events);
	});
	task = std::async(std::launch::async, [this]() { job(); });
}

void worker::on_accept(ekutils::descriptor &, std::uint32_t) {
	auto & client = clients.emplace_front(listener->accept_virtual(), multiplexor);
	log_verbose("new client " + client.sock().remote_endpoint().to_string());
	auto & sock = client.sock();
	sock.set_non_block();
	const auto & it = clients.cbegin();
	log_debug("client " + sock.remote_endpoint().to_string() + " is on thread #" + std::to_string(thread_id()));
	using namespace ekutils::actions;
	multiplexor.add(sock, in | out | et | err | rdhup, [this, &client, it](ekutils::descriptor &, std::uint32_t events) {
		client.on_from_event(events);
		if (client.is_disconnected()) {
			log_verbose("client " + client.sock().remote_endpoint().to_string() + " disconnected");
			clients.erase(it);
		}
	});
}

void worker::on_event(ekutils::descriptor &, std::uint32_t e) {
	log_debug("worker event occurs");
	if (e & ekutils::actions::in) {
		auto event = events.read();
		if (event) switch (event->type()) {
			case worker_event::event_t::stop:
				working = false;
				log_debug("disconnecting clients...");
				for (portal & c : clients) {
					c.on_disconnect();
				}
				break;
			default:
				throw std::runtime_error("undefined worker event type");
		}
	}
}

void worker::job() {
	log_debug("thread spawned");
	while (working) {
		try {
			multiplexor.wait(-1);
		} catch (const std::exception & e) {
			log_error("error on thread #" + std::to_string(thread_id()));
			log_error(e);
		} catch (...) {
			log_error("unknown error on thread #" + std::to_string(thread_id()));
		}
	}
}

std::future<void> & worker::stop() {
	events.write_event(worker_events::stop);
	return task;
}

thread_controller::thread_controller() :
	workers(std::vector<worker>(conf_snap()->threads)) {}

void thread_controller::terminate() {
	std::vector<std::reference_wrapper<std::future<void>>> futures;
	for (worker & w : workers)
		futures.push_back(w.stop());
	for (auto & future : futures) {
		future.get().wait();
	}
}

}
