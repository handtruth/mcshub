#include "prog_args.hpp"

#include <cstring>
#include <stdexcept>
#include <limits>

namespace mcshub {

arguments_t arguments;

const char * get_arg_value(char const **& it, const char ** end) {
    const char * curr = *it;
    while(char c = *(curr++)) {
        if (c == '=') {
            return curr;
        }
    }
    if (++it != end) {
        return *it;
    }
    throw std::invalid_argument(std::string("program argument '") + *(it - 1) + "' requiers value");
}

bool is_arg_name(const char * arg, const char * name) {
    size_t n = std::strlen(name);
    return !std::strcmp(arg, name) || (!std::strncmp(arg, name, n) && arg[n] == '=');
}

template <typename T>
T assert_arg_range(long x, const char * name) {
    constexpr T max_value = std::numeric_limits<T>::max();
    if (x < 0 || x > max_value)
        throw std::invalid_argument(std::string("'") + name + "' value do not fit in range [0:" + std::to_string(max_value) + "]");
    return static_cast<T>(x);
}

void arguments_t::parse(int argn, const char ** args) {
    const char ** end = args + argn;
    for (const char ** it = args; it != end; it++) {
        const char * curr = *it;
        if (!std::strncmp(curr, "--", 2)) {
            // full name conf
            const char * arg = curr + 2;
            if (is_arg_name(arg, "confname"))
                confname = get_arg_value(it, end);
            else if (is_arg_name(arg, "status"))
                status = get_arg_value(it, end);
            else if (is_arg_name(arg, "login"))
                login = get_arg_value(it, end);
            else if (is_arg_name(arg, "default-dir"))
                default_srv_dir = get_arg_value(it, end);
            else if (is_arg_name(arg, "domain"))
                domain = get_arg_value(it, end);
            else if (is_arg_name(arg, "mcsman"))
                mcsman = true;
            else if (is_arg_name(arg, "install"))
                install = true;
            else if (is_arg_name(arg, "distributed"))
                distributed = true;
            else if (is_arg_name(arg, "distributed"))
                drop = true;
            else if (is_arg_name(arg, "address"))
                address = get_arg_value(it, end);
            else if (is_arg_name(arg, "port"))
                port = assert_arg_range<std::uint16_t>(std::atoi(get_arg_value(it, end)), "port");
            else if (is_arg_name(arg, "default-port"))
                default_port = assert_arg_range<std::uint16_t>(std::atoi(get_arg_value(it, end)), "default-port");
            else if (is_arg_name(arg, "max-packet-size"))
                max_packet_size = assert_arg_range<int>(std::atoi(get_arg_value(it, end)), "max-packet-size");
            else if (is_arg_name(arg, "timeout"))
                max_packet_size = assert_arg_range<int>(std::atoi(get_arg_value(it, end)), "timeout");
            else if (is_arg_name(arg, "version"))
                version = true;
            else if (is_arg_name(arg, "help"))
                help = true;
            else if (is_arg_name(arg, "cli"))
                cli = true;
            else if (is_arg_name(arg, "no-dns-cache"))
                no_dns_cache = true;
            else if (is_arg_name(arg, "threads"))
                threads = assert_arg_range<unsigned>(std::atoi(get_arg_value(it, end)), "threads");
			else if (is_arg_name(arg, "verb"))
				verb = ekutils::str2loglvl(get_arg_value(it, end));
            else
                throw std::invalid_argument("urecognized argument '" + std::string(arg) + "'");
        } else if (*curr == '-') {
            // literal options
            const char * arg = curr + 1;
            while(char c = *(arg++)) {
                if (c == 'i')
                    install = true;
                else if (c == 'm')
                    mcsman = true;
                else if (c == 'd')
                    distributed = true;
                else if (c == 'v')
                    version = true;
                else if (c == 'h')
                    help = true;
                else if (c == 'c')
                    cli = true;
                else
                    throw std::invalid_argument(std::string("unrecognized option '") + c + "'");
            }
        } else {
            other.emplace_back(curr);
        }
    }
}

} // mcshub
