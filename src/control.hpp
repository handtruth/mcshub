#ifndef CONTROL_HEAD_SDPXNGPQKGYZFG
#define CONTROL_HEAD_SDPXNGPQKGYZFG

#include <memory>

#include <ekutils/stream_socket_d.hpp>
#include <ekutils/listener_socket_d.hpp>
#include <ekutils/epoll_d.hpp>
#include <ekutils/idgen.hpp>

#include "gate.hpp"

namespace mcshub {

typedef std::unique_ptr<ekutils::stream_socket_d> stream_sock;

class control_port final {
	std::unique_ptr<ekutils::listener_socket_d> server;
	ekutils::epoll_d & epoll;
	std::vector<std::string> whitelist;
public:
	control_port(ekutils::epoll_d & epoll_d, const std::string & link);
	~control_port();
};

class control_client {
	static ekutils::idgen<int> ids;
	ekutils::epoll_d & epoll;
	gate passage;
	int id;
	bool disconnected;
	bool connected;
public:
	control_client(ekutils::epoll_d & epoll_d, stream_sock && c);
	void on_event(std::uint32_t events);
	ekutils::stream_socket_d & sock() {
		return *passage.sock;
	}
	bool is_disconnected() const {
		return disconnected;
	}
private:
	void process_request();
	void process_handshake();
	bool process_next();
	void disconnect() noexcept {
		disconnected = true;
	}
};

} // namespace mcshub

#endif // CONTROL_HEAD_SDPXNGPQKGYZFG
