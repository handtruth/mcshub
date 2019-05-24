#include "test.h"

#include "socket_d.h"
#include "mc_pakets.h"
#include <thread>

const int space_size = 20000;

int main() {
	using namespace mcshub;

	tcp_socket_d sock;
	sock.open(connection_info::resolve("vanilla.mc.handtruth.com", 25565));

	pakets::handshake hs;

	hs.address() = "vanilla.mc.handtruth.com";
	hs.port() = 25565;
	hs.version() = 477;
	hs.state() = 1;

	byte_t space[space_size];

	int s = hs.write(space, space_size);
	sock.write(space, s);

	pakets::request req;

	s = req.write(space, space_size);
	sock.write(space, s);

	pakets::response resp;

	using namespace std::chrono_literals;
	std::this_thread::sleep_for(2s);

	s = sock.read(space, space_size);
	log_info("read: " + std::to_string(s));
	
	assert_equals(false, s < 0);
	
	s = resp.read(space, space_size);
	assert_equals(false, s < 0);

	log_info(resp.message());

	sock.close();

	return 0;
}
