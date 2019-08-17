#include "socket_d.h"

#include <unistd.h>
#include <netdb.h>

#include <cstring>
#include <stdexcept>

#include "putil.h"

namespace mcshub {

std::string endpoint_info::address() const {
    switch (info.addr.sa_family) {
	case AF_INET: {
		const int length = 20;
		char buffer[length];
		inet_ntop(AF_INET, &(info.addr_in.sin_addr), buffer, length);
        return buffer;
	}
	case AF_INET6: {
		const int length = 256;
		char buffer[length];
		inet_ntop(AF_INET6, &(info.addr_in6.sin6_addr), buffer, length);
        return buffer;
	}
	default:
		throw std::runtime_error("unexpected address family accepted: " + std::to_string(info.addr.sa_family));
	}
}

tcp_socket_d::tcp_socket_d(int fd, const endpoint_info & local, const endpoint_info & remote) :
    local_info(local), remote_info(remote) {
    handle = fd;
}

std::vector<connection_info> connection_info::resolve(const std::string & address, const std::string & port) {
	addrinfo initial, *sysaddr = nullptr, *a = nullptr;
	std::memset(&initial, 0, sizeof(addrinfo));
	initial.ai_family = AF_UNSPEC;
	initial.ai_socktype = SOCK_STREAM;
	
	finnaly({
		if (sysaddr != nullptr)
			freeaddrinfo(sysaddr);
	});

	if (int s = getaddrinfo(address.c_str(), port.c_str(), &initial, &sysaddr)) {
		throw dns_error("failed to resolve socket address \"" + address + ':' + port + "\" (getaddrinfo: " + std::string(gai_strerror(s)) + ")");
	}
	std::vector<connection_info> result;
	for (a = sysaddr; a != nullptr; a = a->ai_next) {
		connection_info & info = result.emplace_back();
    	std::memcpy(&(info.endpoint.info.addr), a->ai_addr, a->ai_addrlen);
		info.protocol = a->ai_protocol;
		info.sock_type = a->ai_socktype;
	}
	result.shrink_to_fit();
	return result;
}

void tcp_socket_d::open(const std::vector<connection_info> & infos, bool async) {
	close();
	for (const connection_info & info : infos) {
		handle = socket((int)info.endpoint.addr_family(), info.sock_type | (async ? SOCK_NONBLOCK : 0), info.protocol);
		if (handle == -1)
			continue;
		if (connect(handle, &info.endpoint.info.addr, info.endpoint.addr_len()) != -1 || errno == EINPROGRESS) {
			remote_info = info.endpoint;
			return;
		}
		::close(handle);
	}
	throw std::runtime_error("unable to create socket");
}

std::errc tcp_socket_d::ensure_connected() {
	socklen_t len = sizeof(struct sockaddr_in6);
	if (getpeername(handle, &local_info.info.addr, &len) == -1)
		return std::errc(errno);
	return std::errc(0);
}

std::string tcp_socket_d::to_string() const noexcept {
	return "tcp socket (" + std::string(local_info) + " <-> " + std::string(remote_info) + ")";
}

std::errc tcp_socket_d::last_error() {
	int err;
	socklen_t sz = sizeof(err);
	if (getsockopt(handle, SOL_SOCKET, SO_ERROR, &err, &sz) == -1)
		return std::errc(errno);
	return std::errc(err);
}

int tcp_socket_d::read(byte_t bytes[], size_t length) {
    int r=recv(handle, bytes, length, 0);
    if (r < 0 && errno != EWOULDBLOCK)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to read from socket \"" + std::string(remote_info) + "\"");
    return r;
}

int tcp_socket_d::write(const byte_t bytes[], size_t length) {
    int r = ::write(handle, bytes, length);
    if (r < 0 && errno != EWOULDBLOCK)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to write to socket \"" + std::string(remote_info) + "\"");
    return r;
}

tcp_socket_d::~tcp_socket_d() {}

int open_listener(const std::string & address, std::uint16_t port, endpoint_info & local_info, int sock_type) {
	addrinfo initial, *sysaddr = nullptr, *a = nullptr;
	std::memset(&initial, 0, sizeof(addrinfo));
	initial.ai_family = AF_UNSPEC;
	initial.ai_socktype = sock_type;
	initial.ai_flags = AI_PASSIVE;
	std::string port_str = std::to_string(port);
    int handle;
	finnaly({
		if (sysaddr != nullptr)
			freeaddrinfo(sysaddr);
	});
	if (int s = getaddrinfo(address.c_str(), port_str.c_str(), &initial, &sysaddr)) {
		throw std::runtime_error("failed to create listener (getaddrinfo: " + std::string(gai_strerror(s)) + ")");
	}
	for (a = sysaddr; a != nullptr; a = a->ai_next) {
		handle = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
		/* TODO: Сделай нормально, сначала сокет сщздаётся, затем настраивается, затем открывается */
		int opt = 1;
		setsockopt(handle, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
		if (handle < 0)
			continue;
		if (!bind(handle, a->ai_addr, a->ai_addrlen))
			break;
		close(handle);
	}

	if (a == NULL)
		throw std::runtime_error("can't find any free socket to bind for \"" + address + ':' + port_str + '"');
    std::memcpy(&(local_info.info.addr), a->ai_addr, a->ai_addrlen);
    return handle;
}

tcp_listener_d::tcp_listener_d() {
	handle = -1;
}

tcp_listener_d::tcp_listener_d(const std::string & address, std::uint16_t port, int backlog) {
	handle = -1;
	listen(address, port, backlog);
}

void tcp_listener_d::listen(const std::string & address, std::uint16_t port, int backlog) {
	close();
	this->backlog = backlog;
	handle = open_listener(address, port, local_info, SOCK_STREAM);
}

void tcp_listener_d::start() {
	if (::listen(handle, backlog) < 0)
		throw std::runtime_error(std::string("failed to start tcp listener: ") + std::strerror(errno));
}

std::string tcp_listener_d::to_string() const noexcept {
	return "tcp listener (" + std::string(local_info) + ')';
}

void sockaddr2endpoint(const sockaddr * info, endpoint_info & endpoint) {
    switch (info->sa_family) {
	case AF_INET: {
        std::memcpy(&(endpoint.info.addr_in), info, sizeof(sockaddr_in));
        break;
	}
	case AF_INET6: {
        std::memcpy(&(endpoint.info.addr_in6), info, sizeof(sockaddr_in6));
        break;
	}
	default:
		throw std::runtime_error("unexpected address family accepted: " + std::to_string(info->sa_family));
	}
}

tcp_socket_d tcp_listener_d::accept() {
	static const socklen_t sockaddr_len = sizeof(sockaddr_in6);
	byte_t sockaddr_data[sockaddr_len];
	sockaddr * sockaddr_info = reinterpret_cast<sockaddr *>(sockaddr_data);
	socklen_t actual_len = sockaddr_len;
	int client = ::accept(handle, sockaddr_info, &actual_len);
	if (client < 0)
		throw std::runtime_error(std::string("failed to accept the tcp socket: ") + std::strerror(errno));
	endpoint_info socket_endpoint;
	sockaddr2endpoint(sockaddr_info, socket_endpoint);
	return tcp_socket_d(client, local_info, socket_endpoint);
}

void tcp_listener_d::set_reusable(bool reuse) {
	int opt = reuse ? 1 : 0;
	if (setsockopt(handle, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
		throw std::system_error(std::make_error_code(std::errc(errno)), "can't set SO_REUSEPORT option for tcp listener");
	}
}

tcp_listener_d::~tcp_listener_d() {}

udp_server_d::udp_server_d(const std::string & address, std::uint16_t port) {
    handle = open_listener(address, port, local_info, SOCK_DGRAM);
}

std::string udp_server_d::to_string() const noexcept {
    return "udp server (" + std::string(local_info) + ')';
}

int udp_server_d::read(byte_t bytes[], size_t length, endpoint_info * remote_endpoint) {
	static const socklen_t sockaddr_len = sizeof(sockaddr_in6);
	byte_t sockaddr_data[sockaddr_len];
	sockaddr * sockaddr_info = reinterpret_cast<sockaddr *>(sockaddr_data);
	socklen_t actual_len = sockaddr_len;
    int r = recvfrom(handle, bytes, length, 0, sockaddr_info, &actual_len);
    if (r < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to read from udp socket");
    if (remote_endpoint)
        sockaddr2endpoint(sockaddr_info, *remote_endpoint);
    return r;
}

int udp_server_d::write(const byte_t bytes[], size_t length, const endpoint_info & remote_endpoint) {
    int r = sendto(handle, bytes, length, 0, &(remote_endpoint.info.addr), sizeof(remote_endpoint.info));
    if (r < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to send by udp socket");
    return r;
}

udp_server_d::~udp_server_d() {}

} // mcshub
