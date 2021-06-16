#ifndef GATE_HEAD_QPVOLEQSWDFCDV
#define GATE_HEAD_QPVOLEQSWDFCDV

#include <ekutils/expandbuff.hpp>
#include <ekutils/socket_d.hpp>

namespace mcshub {

using ekutils::byte_t;

typedef std::unique_ptr<ekutils::net::stream_socket_d> stream_sock;

class bad_request : public std::runtime_error {
public:
	explicit bad_request(const std::string & message) : std::runtime_error(message) {}
	explicit bad_request(const char * message) : std::runtime_error(message) {}
};

class gate {
public:
private:
	ekutils::expandbuff input, output;
public:
	stream_sock sock;
	gate() {}
	explicit gate(stream_sock && socket) : sock(std::move(socket)) {}
	bool head(std::int32_t & id, std::int32_t & size) const;
	template <typename P>
	bool paket_read(P & packet);
	template <typename P>
	void paket_write(const P & packet);
	void kostilA();
	void kostilB(const std::string & nick);
	void tunnel(gate & other);
	void receive();
	void send();
	std::size_t avail_read() const noexcept {
		return input.size();
	}
	std::size_t avail_write() const noexcept {
		return output.size();
	}
};

template <typename P>
bool gate::paket_read(P & packet) {
	std::int32_t id, size;
	if (!head(id, size))
		return false;
	int s = packet.read(input.data(), size);
	if (std::int32_t(s) != size)
		throw bad_request("packet format error " + std::to_string(s) + " " + std::to_string(size));
	input.move(size);
	return true;
}

template <typename P>
void gate::paket_write(const P & packet) {
	std::size_t size = 10 + packet.size();
	output.asize(size);
	int s = packet.write(output.data() + output.size() - size, size);
#	ifdef DEBUG
		if (s < 0)
			throw "IMPOSSIBLE SITUATION";
#	endif
	output.ssize(size - s);
	send(); // Init transmission
}

} // namespace mcshub

#endif // GATE_HEAD_QPVOLEQSWDFCDV
