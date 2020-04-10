#include "sclient.hpp"

namespace mcshub {

void sclient::read(std::size_t length) {
    in_buff.asize(length);
    auto ptr = in_buff.data() + in_buff.size() - length;
    for (std::size_t received = 0; received < length;
        received += sock.read(ptr + received, length - received));
}

void sclient::write(const ekutils::byte_t data[], std::size_t length) {
    for (std::size_t received = 0; received < length;
        received += sock.write(data + received, length - received));
}

void sclient::peek_head(std::size_t & size, std::int32_t & id) {
    using namespace handtruth::pakets;
    int actual;
    std::int32_t sz;
    while ((actual = head(in_buff.data(), in_buff.size(), sz, id)) == -1) {
        read(1);
    }
    size = static_cast<std::size_t>(actual) + sz;
}

pakets::response sclient::status(const std::string & name, std::uint16_t port) {
    pakets::handshake hs;
    hs.version() = -1;
    hs.address() = name;
    hs.port() = port;
    hs.state() = 1;
    write_paket(hs);
    pakets::request req;
    write_paket(req);
    pakets::response res;
    read_paket(res);
    return res;
}

std::chrono::milliseconds sclient::ping(std::int64_t & payload) {
    using namespace std::chrono;
    pakets::pinpong pp;
    pp.payload() = payload;
    auto start = std::chrono::system_clock::now();
    write_paket(pp);
    read_paket(pp);
    auto end = std::chrono::system_clock::now();
    return duration_cast<milliseconds>(end-start);
}

pakets::disconnect sclient::login(const std::string & name, std::uint16_t port, const std::string & login) {
    pakets::handshake hs;
    hs.version() = -1;
    hs.address() = name;
    hs.port() = port;
    hs.state() = 2;
    write_paket(hs);
    pakets::login log;
    log.name() = login;
    write_paket(log);
    pakets::disconnect ds;
    read_paket(ds);
    return ds;
}

} // namespace sclient
