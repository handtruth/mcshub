#include "thread_controller.h"

#include <stdexcept>

#include "settings.h"

namespace mcshub {

worker_events::event::~event() {}

void worker_events::redo_write_attempt() {
	if (fifo.empty())
		return;
	try {
		event * ev = fifo.front();
		event_d::write(std::uint64_t(ev));
		fifo.pop_front();
	} catch (const std::system_error & e) {
		if (e.code().value() == int(std::errc::operation_would_block)) {
			log_fatal("event manager is not expected to be used between threads, unexpected situation");
		}
		throw;
	}
}

worker::worker() {
	working = true;
	conf_snap c;
	listener.listen(c->address, c->port);
	//listener.set_reusable();
	listener.start();
	poll.add(listener, [this](descriptor & fd, std::uint32_t events) {
		on_accept(fd, events);
	});
	events.set_non_block();
	using namespace actions;
	poll.add(events, in | out | et, [this](descriptor & fd, std::uint32_t events) {
		on_event(fd, events);
	});
	task = std::async(std::launch::async, [this]() { job(); });
}

void worker::on_accept(descriptor &, std::uint32_t) {
	auto & client = clients.emplace_front(listener.accept(), poll);
	log_verbose("new client " + std::string(client.sock().remote_endpoint()));
	using namespace actions;
	auto & sock = client.sock();
	sock.set_non_block();
	const auto & it = clients.cbegin();
	std::hash<std::thread::id> hasher;
	log_debug("client " + std::string(sock.remote_endpoint()) + " is on thread #" + std::to_string(hasher(std::this_thread::get_id())));
	poll.add(sock, in | out | et | err | rdhup, [this, &client, it](descriptor & fd, std::uint32_t events) {
		client.on_from_event(events);
		if (client.is_disconnected()) {
			log_verbose("client " + std::string(client.sock().remote_endpoint()) + " disconnected");
			clients.erase(it);
		}
	});
}

void worker::on_event(descriptor &, std::uint32_t e) {
	log_debug("worker event occurs");
	if (e & actions::in) {
		while (auto event = events.read()) {
			switch (event->type()) {
				case worker_events::event_t::noop:
					break;
				case worker_events::event_t::stop:
					working = false;
					for (portal & c : clients) {
						c.on_disconnect();
					}
					break;
				default:
					throw std::runtime_error("undefined worker event type");
			}
		}
	}
	if (e & actions::out) {
		events.redo_write_attempt();
	}
}

void worker::job() {
	log_debug("thread spawned");
	while (working) {
		try {
			poll.pull(-1);
		} catch (...) {}
	}
}

std::future<void> & worker::stop() {
	events.write<worker_events::stop>();
	return task;
}

thread_controller::thread_controller() {
	conf_snap c;
	workers = std::vector<worker>(c->threads);
}

void thread_controller::terminate() {
	std::vector<std::reference_wrapper<std::future<void>>> futures;
	for (worker & w : workers) {
		futures.push_back(w.stop());
	}
	for (auto & future : futures) {
		future.get().wait();
	}
}

}
