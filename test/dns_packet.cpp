#include "dns_server.h"
#include "test.h"
#include <iostream>
#include <fstream>
#include <string>

int main() {
    using namespace mcshub;
    dns_packet::question_t q;
    q.name = "lol.kek";
    q.qclass = dns_packet::class_t::IN;
    q.type = dns_packet::record_t::A;
    byte_t b[512];
    int s = q.write(b, 512);
    //std::ofstream file("dns_packet.txt");
    //file.write(reinterpret_cast<const char *>(b), s);
    std::cout << "PACKET SIZE: " << s << std::endl;
    return 0;
}
