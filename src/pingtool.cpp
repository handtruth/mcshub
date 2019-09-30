#include <string>
#include <cinttypes>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <chrono>

#include <netdb.h>

#include <ekutils/socket_d.hpp>
#include <ekutils/expandbuff.hpp>

#include "config.hpp"
#include "mc_pakets.hpp"

struct host_error : public std::runtime_error {
	explicit host_error(const std::string & message) : std::runtime_error(message) {}
};

struct host_info {
	std::string host;
	std::string port;
	host_info(const char * str) {
		switch (*str) {
		case '\0':
			return;
		case '[': {
			const char * begin = str;
			while (*str != ']') {
				if (*str == '\0')
					throw host_error("ipv6 square bracket '[' not closed");
				str++;
			}
			host.append(begin + 1, str - begin - 1);
			if (*str == ']')
				str++;
			break;
		}
		default: {
			const char * begin = str;
			while (*str != ':' && *str) str++;
			host.append(begin, str - begin);
			
			break;
		}
		}
		if (*str) {
			str++;
			port = str;
		} else {
			port = "25565";
		}
	}
};

void print_usage(std::ostream & output, const char * prog) {
	output << "Usage: " << prog << R"==( <option> | <address[:port] [action]>
Options:
  -h, --help     display this help message and exit
  -v, --version  print version number and exit
Actions:
  status         fetch JSON status object
  ping           ping server
)==";
}

enum class acts_enum {
	status, ping
};

void entry(ekutils::tcp_socket_d & sock, host_info & host, acts_enum act);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "1 argument required at least" << std::endl;
		print_usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	if (argc == 2) {
		std::string opt = argv[1];
		if (opt == "-v" || opt == "--version") {
			std::cout << config::build << std::endl;
			return 0;
		} else if (opt == "-h" || opt == "--help") {
			print_usage(std::cout, argv[0]);
			return 0;
		}
	}
	if (argc >= 4) {
		std::cerr << "maximum 2 arguments are allowed" << std::endl;
		print_usage(std::cerr, argv[0]);
		return EXIT_FAILURE;
	}
	acts_enum act = acts_enum::status;
	if (argc == 3) {
		std::string opt = argv[2];
		if (opt == "status")
			act = acts_enum::status;
		else if (opt == "ping")
			act = acts_enum::ping;
		else {
			std::cerr << "only status and ping actions supported" << std::endl;
			print_usage(std::cerr, argv[0]);
			return EXIT_FAILURE;
		}
	}
	try {
		host_info host(argv[1]);

		ekutils::tcp_socket_d sock;
		auto cons = ekutils::connection_info::resolve(host.host, host.port);
		sock.open(cons);
		entry(sock, host, act);
	} catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void ensure_write(ekutils::out_stream & output, const ekutils::byte_t bytes[], std::size_t length) {
	for (std::size_t written = 0;
		written != length;
		written += output.write(bytes + written, length - written));
}

void ensure_read(ekutils::in_stream & input, ekutils::byte_t bytes[], std::size_t length) {
	for (std::size_t received = 0;
		received != length;
		received += input.read(bytes + received, length - received));
}

template <typename P>
void read_paket(P & paket, ekutils::in_stream_d & input, ekutils::expandbuff & buff) {
	using namespace mcshub;
	int s;
	while ((s = paket.read(buff.data(), buff.size())) == -1) {
		buff.asize(1);
		int s = input.read(buff.data() + buff.size() - 1, 1);
		assert(s == 1);
		std::size_t avail = input.avail();
		buff.asize(avail);
		s = input.read(buff.data() + buff.size() - avail, avail);
		assert(std::size_t(s) == avail);
	}
	buff.move(s);
}

void entry(ekutils::tcp_socket_d & sock, host_info & host, acts_enum act) {
	using namespace mcshub;
	pakets::handshake hs;
	hs.version() = -1;
	hs.address() = host.host;
	hs.port() = sock.remote_endpoint().port();
	hs.state() = 1;
	std::array<ekutils::byte_t, 1000> send;
	std::size_t tosend = hs.write(send);
	ensure_write(sock, send.data(), tosend);
	pakets::request req;
	tosend = req.write(send);
	ensure_write(sock, send.data(), tosend);
	pakets::response resp;
	ekutils::expandbuff buff;
	read_paket(resp, sock, buff);
	if (act == acts_enum::status) {
		std::cout << resp.message() << std::endl;
		return;
	}
	pakets::pinpong pp;
	pp.payload() = random();
	tosend = pp.write(send);
	auto start = std::chrono::system_clock::now();
	ensure_write(sock, send.data(), tosend);
	read_paket(pp, sock, buff);
    auto end = std::chrono::system_clock::now();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << millis.count() << "ms" << std::endl;
}
