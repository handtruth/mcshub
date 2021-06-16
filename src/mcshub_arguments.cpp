#include "mcshub_arguments.hpp"

#include <cstring>
#include <stdexcept>
#include <limits>

namespace mcshub {

arguments_t arguments;

arguments_t::arguments_t() :
confname(add<string>("confname", [](string & opt) {
    opt.value = "mcshub.yml";
    opt.hint = "change configuration file name";
})),
status(add<string>("status", [](string & opt) {
    opt.value = "status.json";
    opt.hint = "change default status file name";
})),
login(add<string>("login", [](string & opt) {
    opt.value = "login.json";
    opt.hint = "change default login file name";
})),
default_srv_dir(add<string>("default-dir", [](string & opt) {
    opt.value = "default";
    opt.hint = "specify name of a directory for the default entry";
})),
domain(add<string>("domain", [](string & opt) {
    opt.hint = "specify domain name (the same option as in mcshub.yml file). "
        "This option will be overriden by \"domain\" field in mcshub.yml file";
})),
mcsman(add<flag>("mcsman", [](flag & opt) {
    opt.c = 'm';
    opt.hint = "enable extra functionality for MCSMan";
})),
install(add<flag>("install", [](flag & opt) {
    opt.c = 'i';
    opt.hint = "install the default MCSHub configuration";
})),
distributed(add<flag>("distributed", [](flag & opt) {
    opt.c = 'd';
    opt.hint = "enable distributed MCSHub configuration";
})),
drop(add<flag>("drop", [](flag & opt) {
    opt.hint = "drop if no server entry matches";
})),
address(add<string>("address", [](string & opt) {
    opt.value = "tcp://0.0.0.0:25565";
    opt.hint = "address and port to listen connections from";
})),
default_port(add<integer>("default-port", [](integer & opt) {
    opt.value = 25565;
    opt.hint = "set the default TCP port number for all server records in configuration file";
    opt.min = 0;
    opt.max = 65535;
})),
max_packet_size(add<integer>("max-packet-size", [](integer & opt) {
    opt.value = 6000;
    opt.hint = "set maximum allowed packet size for Minecraft protocol";
    opt.min = 0;
})),
timeout(add<integer>("timeout", [](integer & opt) {
    opt.value = 500;
    opt.hint = "set timeout";
    opt.min = 0;
})),
version(add<flag>("version", [](flag & opt) {
    opt.c = 'v';
    opt.hint = "print version number and exit";
})),
help(add<flag>("help", [](flag & opt) {
    opt.c = 'h';
    opt.hint = "display this help message and exit";
})),
cli(add<flag>("cli", [](flag & opt) {
    opt.c = 'c';
    opt.hint = "enable command line interface";
})),
no_dns_cache(add<flag>("no-dns-cache", [](flag & opt) {
    opt.hint = "disable dns cache by default. This option will be overriden by 'dns_cache' option in configuration file";
})),
threads(add<integer>("threads", [](integer & opt) {
    opt.value = -1;
    opt.hint = "number of working theads. (option 'threads' in configuration "
        "file, default value equals to number of parallel threads supported by CPU)";
})),
verb(add<variant<ekutils::log_level>>("verb", [](variant<ekutils::log_level> & opt) {
    opt.hint = "set default verbose level to VARIANT. This option will "
        "be overriden by 'verb' option in configuration file.";
    opt.value = ekutils::log_level::info;
    opt.variants = {
        { "none", ekutils::log_level::none },
        { "fatal", ekutils::log_level::fatal },
        { "error", ekutils::log_level::error },
        { "warning", ekutils::log_level::warning },
        { "info", ekutils::log_level::info },
        { "verbose", ekutils::log_level::verbose },
        { "debug", ekutils::log_level::debug }
    };
})),
control(add<multistring>("control", [](multistring & opt) {
    opt.hint = "management socket address";
}))
{
    allow_positional = false;
    width = 60;
}

} // mcshub
