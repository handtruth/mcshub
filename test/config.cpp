#include "settings.h"
#include "log.h"
#include <iostream>

void print_record(const mcshub::config::server_record & record) {
    using namespace std;
    using namespace mcshub;
    static auto tab = string("\t");
    log->info(tab + "address " + record.address);
    log->info(tab + "port " + to_string(record.port));
    log->info(tab + "status " + record.status);
    log->info(tab + "login " + record.login);
    string result = tab + "vars: { ";
    for (auto item : record.vars) {
        result += item.first + "=" + item.second + ' ';
    }
    result += "}";
    log->info(result);
}

int main(int argc, char *argv[]) {
    using namespace mcshub;
    using namespace std;
    config conf;
    conf.load("config.yml");
    stdout_log std_log(log_level::info);
    log = &std_log;

    log->info("address " + std::string(conf.address));
    log->info("port " + to_string(conf.port));
    log->info("threads " + to_string(conf.threads));
    log->info("log " + std::string(conf.log));
    log->info("verb " + log_lvl2str(conf.verb));
    log->info("default:");
    print_record(conf.default_server);
    log->info("servers:");
    for (const auto record : conf.servers) {
        log->info(record.first + ":");
        print_record(record.second);
    }
    return 0;
}
