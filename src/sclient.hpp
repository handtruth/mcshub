#ifndef _SCLIENT_HEAD
#define _SCLIENT_HEAD

#include <cassert>
#include <chrono>

#include <ekutils/socket_d.hpp>
#include <ekutils/putil.hpp>
#include <ekutils/expandbuff.hpp>

#include "mc_pakets.hpp"

namespace mcshub {

class sclient {
	std::vector<ekutils::connection_info> conns;
	ekutils::tcp_socket_d sock;
	ekutils::expandbuff in_buff;
	static std::uint64_t timestamp() {
		using namespace std::chrono;
		return system_clock::to_time_t(system_clock::now());
	}
	void read(std::size_t length);
	void write(const ekutils::byte_t data[], std::size_t length);
public:
	sclient(const std::string & host, const std::string & service = "minecraft") :
		conns(ekutils::connection_info::resolve(host, service)), sock(conns) {}
	sclient(const std::string & host, std::uint16_t port = 25565) :
		conns(ekutils::connection_info::resolve(host, port)), sock(conns) {}
	void peek_head(std::size_t & size, std::int32_t & id);
	template <typename P>
	void read_paket(P & paket) {
		using namespace handtruth::pakets;
		using namespace ekutils;
		std::size_t size;
		std::int32_t id;
		peek_head(size, id);
		read(size - in_buff.size());
		int received = paket.read(in_buff.data(), size);
		assert(std::size_t(received) == size);
		in_buff.move(size);
	}
	template <typename P>
	void write_paket(const P & paket) {
		using namespace handtruth::pakets;
		using namespace ekutils;
		std::size_t size = paket.size();
		size = size + size_varint(paket.id()) + size_varint(size);
		auto data = new byte_t[size];
		finnaly({
			delete[] data;
		});
		int written = paket.write(data, size);
		assert(std::size_t(written) == size);
		write(data, size);
	}
	void reconnect() {
		sock.open(conns);
	}
	pakets::response status(const std::string & name, std::uint16_t port);
	pakets::response status(const std::string & name) {
		return status(name, sock.remote_endpoint().port());
	}
	std::chrono::milliseconds ping(std::int64_t & payload);
	std::chrono::milliseconds ping() {
		std::int64_t payload = timestamp();
		return ping(payload);
	}
	pakets::disconnect login(const std::string & name, std::uint16_t port, const std::string & login);
};

} // namespace sclient

#endif // _SCLIENT_HEAD
