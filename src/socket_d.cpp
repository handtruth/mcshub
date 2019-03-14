#include "socket_d.h"

#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <cstring>
#include <stdexcept>
#include <system_error>

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

std::string tcp_socket_d::name() const noexcept {
	return "tcp socket (" + std::string(local_info) + " <-> " + std::string(remote_info) + ")";
}

size_t tcp_socket_d::avail() const {
	std::size_t size;
	if (ioctl(handle, FIONREAD, &size) == -1)
		throw std::system_error(std::make_error_code(std::errc(errno)),
            "error while getting available bytes");
	return size;
}

int tcp_socket_d::read(byte_t bytes[], size_t length) {
    int r=recv(handle, bytes, length, 0);
    if (r < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to read from socket \"" + std::string(remote_info) + "\"");
    return r;
}

int tcp_socket_d::write(const byte_t bytes[], size_t length) {
    int r = ::write(handle, bytes, length);
    if (r < 0)
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
	if (int s = getaddrinfo(address.c_str(), port_str.c_str(), &initial, &sysaddr)) {
		if (sysaddr != nullptr)
			freeaddrinfo(sysaddr);
		throw std::runtime_error("failed to create listener (getaddrinfo: " + std::string(gai_strerror(s)) + ")");
	}
	for (a = sysaddr; a != nullptr; a = a->ai_next) {
		handle = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
		if (handle < 0)
			continue;
		if (!bind(handle, a->ai_addr, a->ai_addrlen))
			break;
		close(handle);
	}

	if (a == NULL)
		throw std::runtime_error("can't find any free socket to bind for \"" + address + ':' + port_str + '"');
		freeaddrinfo(sysaddr);
    std::memcpy(&(local_info.info.addr), a->ai_addr, a->ai_addrlen);
    //if (a->ai_family == AF_INET)
    //    local_info.info.addr_in.sin_port = port;
    //else
    //    local_info.info.addr_in6.sin6_port = port;
    return handle;
}

tcp_listener_d::tcp_listener_d(const std::string & address, std::uint16_t port, int backlog) {
	this->backlog = backlog;
	handle = open_listener(address, port, local_info, SOCK_STREAM);
}

void tcp_listener_d::start() {
	if (listen(handle, backlog) < 0)
		throw std::runtime_error(std::string("failed to start tcp listener: ") + std::strerror(errno));
}

std::string tcp_listener_d::name() const noexcept {
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

tcp_listener_d::~tcp_listener_d() {}

udp_server_d::udp_server_d(const std::string & address, std::uint16_t port) {
    handle = open_listener(address, port, local_info, SOCK_DGRAM);
}

std::string udp_server_d::name() const noexcept {
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

int udp_server_d::write(byte_t bytes[], size_t length, const endpoint_info & remote_endpoint) {
    int r = sendto(handle, bytes, length, 0, &(remote_endpoint.info.addr), sizeof(remote_endpoint.info));
    if (r < 0)
        throw std::system_error(std::make_error_code(std::errc(errno)),
            "failed to send by udp socket");
    return r;
}

udp_server_d::~udp_server_d() {}

} // mcshub
