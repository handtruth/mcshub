#include "settings.hpp"

#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <sys/sysinfo.h>

#include <yaml-cpp/yaml.h>
#include <ekutils/primitives.hpp>
#include <ekutils/log.hpp>
#include <ekutils/lazy.hpp>

#include "status_builder.hpp"
#include "mcshub_arguments.hpp"
#include "response_props.hpp"
#include "resources.hpp"

namespace fs = std::filesystem;

namespace mcshub {

using ekutils::byte_t;

const char * std_log = "$std";

const fs::path cdir = ".";

namespace resps {
	template <std::size_t N>
	YAML::Node load(const std::array<byte_t, N> & data) {
		return YAML::Load(std::string(res::view(data)));
	}

	namespace mcsman {
		const ekutils::lazy<strparts> status {
			[]() -> strparts { return build_status(load(res::config::mcsman::status_yml), "@"); }
		};
		const ekutils::lazy<strparts> login {
			[]() -> strparts { return build_chat(load(res::config::mcsman::login_yml), "@"); }
		};
	}

	namespace fallback {
		const ekutils::lazy<strparts> status {
			[]() -> strparts { return build_status(load(res::config::fallback::status_yml), "@"); }
		};
		const ekutils::lazy<strparts> login {
			[]() -> strparts { return build_chat(load(res::config::fallback::login_yml), "@"); }
		};
	}
}

void fill_record(const YAML::Node & node, settings::basic_record & record, const std::string & context);
void fill_srv_record(const YAML::Node & node, settings::server_record & record, const std::string & context);
void fill_settings(const YAML::Node & node, settings & conf);

void put_main_conf_file();

inline settings::server_record conf_record_mcsman(const std::string & name) {
	const std::string domain = name + "-mcs";
	return {
		domain, resps::mcsman::status, resps::mcsman::login,
		false, { { "name", name } }
	};
}

settings default_conf;
settings::basic_record default_record;

ekutils::matomic<std::shared_ptr<const settings>> conf_instance;
const ekutils::matomic<std::shared_ptr<const settings>> & conf = conf_instance;

void load_all_conf(const std::shared_ptr<settings> & c) {
	c->load(arguments.confname);
	// load configurations for all sub directories
	for (const auto & file : fs::directory_iterator(cdir)) {
		if (file.is_directory()) {
			auto & servers = c->servers;
			std::string name = file.path().filename();
			if (arguments.mcsman && name != "default") {
				// load mcsman settings first
				log_info("init mcsman configuration for \"" + name + "\"");
				servers[name] = conf_record_mcsman(name);
			}
			fs::path conf_f = file.path()/arguments.confname;
			if (fs::exists(conf_f) && fs::is_regular_file(conf_f)) {
				// load sub conf
				if (c->distributed) {
					log_info("init distributed configuration for \"" + name + "\"");
					auto node = YAML::LoadFile(conf_f);
					auto iter = servers.find(name);
					if (iter != servers.end()) {
						fill_srv_record(node, iter->second, name);
					} else {
						settings::server_record record = default_record;
						fill_srv_record(node, record, name);
						servers[name] = record;
					}
				}
			}
		}
	}
	file_vars fvars;
	img_vars ivars;
	server_vars svars;
	auto vars = make_vars_manager(main_vars, fvars, ivars, env_vars, svars);
	for (auto & server : c->servers) {
		fvars.srv_name = server.first;
		ivars.srv_name = server.first;
		svars.vars = &server.second.vars;
		server.second.status = server.second.status.resolve_partial(vars);
		server.second.login = server.second.login.resolve_partial(vars);
	}
}

void reload_configuration() {
	auto new_conf = std::make_shared<settings>(default_conf);
	load_all_conf(new_conf);
	conf_instance = new_conf;
}

void settings::initialize() {
	if (!fs::exists(arguments.confname))
		put_main_conf_file();
	default_conf = {
		arguments.address, // address
		arguments.threads == -1 ? unsigned(get_nprocs()) : static_cast<unsigned>(arguments.threads), // threads
		arguments.max_packet_size, // max_packet_size
		static_cast<unsigned long>(arguments.timeout), // timeout
		std_log, // log
		arguments.verb, // verb
		arguments.distributed, // distributed
		arguments.domain, // domain
		{ // default
			"", // address
			resps::fallback::status, // status
			resps::fallback::login, // login
			arguments.drop, // drop
			{}, // vars
		},
		{}, // servers
		!arguments.no_dns_cache // dns_cache
	};
	default_record = {
		std::string(), //address
		resps::fallback::status, // status
		resps::fallback::login, // login
		false, // drop
		{} //vars
	};
	auto c = std::make_shared<settings>(default_conf);
	load_all_conf(c);
	conf_instance = c;
}

ekutils::log_level str2lvl(const std::string & verb) {
	if (verb == "none")
		return ekutils::log_level::none;
	if (verb == "fatal")
		return ekutils::log_level::fatal;
	if (verb == "error")
		return ekutils::log_level::error;
	if (verb == "warning")
		return ekutils::log_level::warning;
	if (verb == "info")
		return ekutils::log_level::info;
	if (verb == "verbose")
		return ekutils::log_level::verbose;
	if (verb == "debug")
		return ekutils::log_level::debug;
	throw config_exception("verb", "no '" + verb + "' log level");
}

void fill_record(const YAML::Node & node, settings::basic_record & record, const std::string & context) {
	if (node.IsScalar()) {
		if (node.as<std::string>() == "drop") {
			record.drop = true;
			return;
		}
		throw config_exception("record", "this field should equals to \"drop\" or server record object");
	} else
		record.drop = false;
	if (const auto address = node["address"])
		record.address = address.as<std::string>();
	for (const auto item : node["vars"])
		record.vars[item.first.as<std::string>()] = item.second.as<std::string>();
	if (const auto status = node["status"])
		record.status = build_status(status, context);
	if (const auto login = node["login"])
		record.login = build_chat(login, context);
}

void fill_srv_record(const YAML::Node & node, settings::server_record & record, const std::string & context) {
	fill_record(node, record, context);
	if (record.drop)
		return;
	if (auto fml = node["fml"]) {
		if (fml.IsScalar()) {
			try {
				if (fml.as<bool>())
					record.fml.reset();
				else
					record.fml = default_record;
			} catch (const YAML::Exception &) {
				if (fml.as<std::string>() == "drop") {
					record.fml = default_record;
					record.fml->drop = true;
					return;
				}
				throw config_exception("record.fml", "only 'true', 'false' or 'drop' are supported as scalar values");
			}
		} else {
			fill_record(fml, record.fml.emplace(default_record), context);
		}
	}
}

void fill_settings(const YAML::Node & node, settings & conf) {
	if (auto address = node["address"])
		conf.address = address.as<std::string>();
	if (auto threads = node["threads"]) {
		try {
			conf.threads = threads.as<unsigned int>();
		} catch (YAML::Exception &) {
			if (threads.as<std::string>() == "$cpu")
				conf.threads = get_nprocs();
		}
	}
	if (auto max_packet_size = node["max_packet_size"]) {
		std::int32_t max_size;
		conf.max_packet_size = max_size = max_packet_size.as<std::int32_t>();
		if (max_size < 0)
			throw config_exception("max_packet_size", "maximum Minecraft packet size is lower than zero");
	}
	if (auto timeout = node["timeout"])
		conf.timeout = timeout.as<unsigned long>();
	if (auto log = node["log"])
		conf.log = log.as<std::string>();
	if (auto verb = node["verb"]) {
		try {
			conf.verb = (ekutils::log_level)verb.as<int>();
		} catch (YAML::Exception &) {
			conf.verb = str2lvl(verb.as<std::string>());
		}
	}
	if (auto distributed = node["distributed"])
		conf.distributed = distributed.as<bool>();
	if (auto domain = node["domain"])
		check_domain(conf.domain = domain.as<std::string>());
	if (auto default_server = node["default"]) {
		if (!default_server.IsMap())
			throw config_exception("default", "not a map yaml structure");
		fill_srv_record(default_server, conf.default_server, ".");
	}
	if (auto servers = node["servers"]) {
		if (!servers.IsMap())
			throw config_exception("servers", "not a map yaml structure");
		for (auto record : servers) {
			settings::server_record server = default_record;
			const std::string & name = record.first.as<std::string>();
			fill_srv_record(record.second, server, name);
			conf.servers[record.first.as<std::string>()] = server;
		}
	}
	if (auto dns_cache = node["dns_cache"])
		conf.dns_cache = dns_cache.as<bool>();
}

void settings::load(const std::string & path) {
	auto node = YAML::LoadFile(path);
	fill_settings(node, *this);
}

void settings::load(std::istream & input) {
	auto node = YAML::Load(input);
	fill_settings(node, *this);
}

void put_main_conf_file() {
	std::ofstream config_file(arguments.confname);
	config_file << res::view(res::config::mcshub_yml);
	config_file.close();
}

void settings::static_install() {
	if (!fs::exists(arguments.default_srv_dir) && !fs::create_directory(arguments.default_srv_dir))
		throw std::runtime_error("can't create directory");
	std::ofstream status_file(arguments.default_srv_dir + '/' + arguments.status);
	status_file << res::view(res::config::fallback::status_yml);
	status_file.close();
	std::ofstream login_file(arguments.default_srv_dir + '/' + arguments.login);
	login_file << res::view(res::config::fallback::login_yml);
	login_file.close();
	put_main_conf_file();
}

void check_domain(std::string & name) {
	if (!name.empty()) {
		if (name == ".")
			name = "";
		else {
			if (name[0] == '.')
				throw config_exception("domain", "domain name can't start with '.'");
			for (std::size_t i = 0, size = name.size(); i < size; i++) {
				char c = name[i];
				if (c != '.' && !std::isalnum(c) && c != '_' && c != '-')
					throw config_exception("domain", "domain name has illegal characters");
			}
			if (name[name.size() - 1] == '.')
				name.pop_back();
		}
	}
}

} // mcshub
