#include "thread_controller.hpp"

#include <stdexcept>

#include "settings.hpp"

namespace mcshub {

worker::worker() : working(true) {
	conf_snap c;
	listener.listen(c->address, c->port, ekutils::tcp_flags::reuse_port);
	listener.start();
	poll.add(listener, [this](ekutils::descriptor & fd, std::uint32_t events) {
		on_accept(fd, events);
	});
	events.set_non_block();
	using namespace ekutils::actions;
	poll.add(events, in | out | et, [this](ekutils::descriptor & fd, std::uint32_t events) {
		on_event(fd, events);
	});
	task = std::async(std::launch::async, [this]() { job(); });
}

void worker::on_accept(ekutils::descriptor &, std::uint32_t) {
	auto & client = clients.emplace_front(listener.accept(), poll);
	log_verbose("new client " + std::string(client.sock().remote_endpoint()));
	auto & sock = client.sock();
	sock.set_non_block();
	const auto & it = clients.cbegin();
	std::hash<std::thread::id> hasher;
	log_debug("client " + std::string(sock.remote_endpoint()) + " is on thread #" + std::to_string(hasher(std::this_thread::get_id())));
	using namespace ekutils::actions;
	poll.add(sock, in | out | et | err | rdhup, [this, &client, it](ekutils::descriptor &, std::uint32_t events) {
		client.on_from_event(events);
		if (client.is_disconnected()) {
			log_verbose("client " + std::string(client.sock().remote_endpoint()) + " disconnected");
			clients.erase(it);
		}
	});
}

void worker::on_event(ekutils::descriptor &, std::uint32_t e) {
	log_debug("worker event occurs");
	if (e & ekutils::actions::in) {
		switch (events.read()) {
			case worker_events::event_t::noop:
				break;
			case worker_events::event_t::stop:
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
			poll.wait(-1);
		} catch (...) {}
	}
}

std::future<void> & worker::stop() {
	events.write(worker_events::event_t::stop);
	return task;
}

thread_controller::thread_controller() :
	workers(std::vector<worker>(conf_snap()->threads)) {}

void thread_controller::terminate() {
	std::vector<std::reference_wrapper<std::future<void>>> futures;
	//std::transform(workers.begin(), workers.end(), futures.begin(), [](worker & w) -> auto & {
	//	return w.stop();
	//});
	for (worker & w : workers)
		futures.push_back(w.stop());
	for (auto & future : futures) {
		future.get().wait();
	}
}

}
