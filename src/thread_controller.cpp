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
	conf_snap c = conf;
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
	task = std::async(std::launch::async, [this](){ job(); });
}

void worker::on_accept(descriptor &, std::uint32_t) {
	auto & client = clients.emplace_front(listener.accept(), *this);
	client.set_me(clients.begin());
	log_verbose("new client " + std::string(client.client_socket().remote_endpoint()));
	using namespace actions;
	auto & sock = client.client_socket();
	sock.set_non_block();
	poll.add(sock, in | out | et | err | rdhup, [this, &client](descriptor & fd, std::uint32_t events) {
		client.on_client_sock(fd, events);
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
					for (client & c : clients) {
						c.terminate();
					}
					break;
				case worker_events::event_t::remove: {
					auto & iter = dynamic_cast<worker_events::remove &>(*event).item;
					log_debug("remove client from list: " + std::string(iter->client_socket().remote_endpoint()));
					clients.erase(dynamic_cast<worker_events::remove &>(*event).item);
					break;
				}
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
	log_debug("thread started");
	while (working) {
		poll.pull(-1);
	}
}

std::future<void> & worker::stop() {
	events.write<worker_events::stop>();
	return task;
}

void worker::remove(const std::list<client>::iterator & item) {
	events.write<worker_events::remove>(item);
}

thread_controller::thread_controller() {
	conf_snap c = conf;
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
