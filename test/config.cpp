#include "settings.hpp"
#include "test.hpp"
#include <ekutils/log.hpp>

#include <sstream>

using ekutils::log;

void print_record(const mcshub::settings::server_record & record) {
    using namespace std;
    using namespace mcshub;
    static auto tab = string("\t");
    log->info(tab + "address " + record.address);
    log->info(tab + "port " + to_string(record.port));
    log->info(tab + "status " + record.status.resolve());
    log->info(tab + "login " + record.login.resolve());
    string result = tab + "vars: { ";
    for (auto item : record.vars) {
        result += item.first + "=" + item.second + ' ';
    }
    result += "}";
    log->info(result);
}

test {
    std::istringstream input(
R"==(
#MCSHub example config
address: 127.0.0.1
port: 25564
threads: 5
log: $other
verb: warning

default: &srv
  address: mc.handtruth.com
  port: 25561
  response: "example/response.json"
  login: "example/login.json"
  log: "latest.log"
  vars:
    lol: kek
    name: May

servers:
  127.0.0.1: *srv
  two:
    address: mc.hypixel.com
    port: 25565
)==");
    using namespace mcshub;
    using namespace std;
    settings conf = default_conf;
    conf.load(input);

    log->info("address " + std::string(conf.address));
    log->info("port " + to_string(conf.port));
    log->info("threads " + to_string(conf.threads));
    log->info("log " + std::string(conf.log));
    log->info("verb " + log_lvl2str(conf.verb));
    log->info("default:");
    //print_record(conf.default_server);
    log->info("servers:");
    for (const auto record : conf.servers) {
        log->info(record.first + ":");
        //print_record(record.second);
    }
}
