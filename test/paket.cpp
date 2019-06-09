#include "mc_pakets.h"
#include <cassert>
#include <iostream>
#include "log.h"
#include "test.h"

int main() {
    using namespace mcshub;

    try {
        const mcshub::size_t sz = 2048;
        byte_t bytes[sz];
////////////////////////////////////
        pakets::request rq1;
        int wrq = rq1.write(bytes, sz);
        mcshub::size_t rq_sz = rq1.size() + size_varint(rq1.size()) + size_varint(rq1.id());
        assert_equals(rq_sz, mcshub::size_t(wrq));
////////////////////////////////////
        pakets::handshake hs1;
        hs1.version() = 64416;
        hs1.address() = "continuum.mc.handtruth.com";
        hs1.port() = 25565;
        hs1.state() = 34;

        int written = hs1.write(bytes, sz);
        mcshub::size_t hs1_sz = hs1.size() + size_varint(hs1.size()) + size_varint(rq1.id());
        assert_equals(hs1_sz, mcshub::size_t(written));
        assert(written > 0);
        pakets::handshake hs2;
        int rsz = hs2.read(bytes, sz);
        assert_equals(written, rsz);
        assert_equals(hs1.version(), hs2.version());
        assert_equals(hs1.address(), hs2.address());
        assert_equals(hs1.port(), hs2.port());
        assert_equals(hs1.state(), hs2.state());
        assert_equals(hs1, hs2);
        log->info("pingpong size: " + std::to_string(sizeof(pakets::pinpong)));
        log->info(std::to_string(hs1) + " = " + std::to_string(hs2));
    } catch (paket_error & e) {
        std::cout << ((std::exception &)e).what() << std::endl;
        throw;
    }

    return 0;
}
