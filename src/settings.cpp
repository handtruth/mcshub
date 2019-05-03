#include "settings.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <algorithm>

#include "hardcoded.h"
#include "log.h"
#include "prog_args.h"
#include "inotify_d.h"
#include "directory_cache.h"
#include "dns_server.h"

namespace fs = std::filesystem;

namespace mcshub {

const char * std_log = "$std";
const char * main_log = "$main";

const fs::path cdir = ".";

inotify_d fs_watcher;
directory_cache dir_cache(cdir);

void operator>>(const YAML::Node & node, config::server_record & record);
void operator>>(const YAML::Node & node, config::dns_module & dns);
void operator>>(const YAML::Node & node, config & conf);
void operator<<(YAML::Node & node, const config::server_record & record);
void operator<<(YAML::Node & node, const config & conf);

config::server_record conf_record_default() {
	return { "", arguments.default_port, "", "", main_log, true };
}

std::shared_ptr<config> conf_default() {
	std::shared_ptr<config> ptr = std::make_shared<config>(config {
		"0.0.0.0", arguments.port, 1, std_log, log_level::info, false, { false },
		conf_record_default()
	});
	return ptr;
}

std::shared_ptr<config> conf_install() {
	std::shared_ptr<config> ptr = std::make_shared<config>(config {
		"0.0.0.0", arguments.port, 1, std_log, log_level::info, false, { false },
		{ "", arguments.default_port, cdir/arguments.default_srv_dir/arguments.status,
			cdir/arguments.default_srv_dir/arguments.login, main_log, true }
	});
	return ptr;
}

matomic<std::shared_ptr<const config>> conf_instance;
const matomic<std::shared_ptr<const config>> & conf = conf_instance;

void config::initialize() {
	auto c = conf_default();
	c->load(arguments.confname);
	conf_instance = c;
	fs_watcher.add_watch(inev::close_write | inev::delete_self, arguments.confname);
	fs_watcher.add_watch((c->distributed ? inev::create : 0) | inev::in_delete, cdir);
	auto content = dir_cache.find_new();
	if (c->distributed) {
		// load configurations for all sub directories
		for (const std::string & name : content) {
			if (fs::is_directory(name)) {
				auto & servers = c->servers;
				if (arguments.mcsman) {
					// load mcsman settings first
					mcshub::log->debug("init mcsman configuration for \"" + name + "\"");
					servers[name] = config::server_record {
						name, arguments.default_port, arguments.status, arguments.login, main_log, false,
						{ { "name", name } }
					};
				}
				fs::path conf = cdir/name/arguments.confname;
				if (fs::exists(conf) && fs::is_regular_file(conf)) {
					// load sub conf
					mcshub::log->debug("init distributed configuration for \"" + name + "\"");
					auto node = YAML::LoadFile(conf);
					auto iter = servers.find(name);
					if (iter != servers.end()) {
						node >> iter->second;
					} else {
						config::server_record record = conf_record_default();
						node >> record;
						servers[name] = record;
					}
				}
			}
		}
	}
}

void config::init_listener(event & poll) {
	fs::path main_conf_file = fs::canonical(arguments.confname);
	fs::path main_dir_path = fs::canonical(cdir);
	poll.add(fs_watcher, [main_conf_file, main_dir_path](descriptor &, std::uint32_t) {
		std::vector<inotify_d::event_t> events = fs_watcher.read();
		mcshub::log->debug("got " + std::to_string(events.size()) + " filesystem events");
		std::shared_ptr<const config> old_conf = conf;
		std::shared_ptr<config> new_conf = std::make_shared<config>(*old_conf);
		for (const inotify_d::event_t & event : events) {
			mcshub::log->debug("fs event info: " + std::string(event.watch.path())
				+ " " + std::string(main_conf_file) + " " + std::string(main_dir_path));
			// 1: main conf { delete_self, close_write }
			// 2: main dir { create, delete }
			// 3: sub cong { delete_self, close_write }
			if (event.watch.path() == main_conf_file) {
				// main conf
				if (event.mask & inev::delete_self) {
					// main conf: delete_self
					mcshub::log->error("main config file was deleted");
				}
				if (event.mask & inev::close_write) {
					// main conf: close_write
					auto node = YAML::LoadFile(main_conf_file);
					node >> *new_conf;
					// allow or disallow distributed configuration
					if (!old_conf->distributed && new_conf->distributed) {
						// distrubuted was on
						fs_watcher.mod_watch(inev::create | inev::in_delete, main_dir_path);
					} else if (old_conf->distributed && !new_conf->distributed) {
						// distrubuted was off
						fs_watcher.mod_watch(inev::in_delete, main_dir_path);
					}
					mcshub::log->info("reloaded main configuration");
				}
			} else if (event.watch.path() == main_dir_path) {
				// main dir
				if (event.mask & inev::create) {
					// main dir: create
					std::vector<std::string> new_files = dir_cache.find_new();
					for (const std::string & name : new_files) {
						if (fs::is_directory(name)) {
							// add sub conf
							fs_watcher.add_watch(inev::delete_self | inev::close_write, cdir/name/arguments.confname);
							if (arguments.mcsman) {
								// add mcsman auto-record
								mcshub::log->info("added new mcsman server configuration \"" + name + "\"");
								new_conf->servers[name] = config::server_record {
									name, arguments.default_port, arguments.status, arguments.login, main_log, false,
									{ { "name", name } }
								};
							}
						}
					}
				}
				if (event.mask & inev::in_delete) {
					// main dir: delete
					std::set<std::string> del_files = dir_cache.find_deleted();
					for (const std::string & name : del_files) {
						if (fs::is_directory(name)) {
							// del sub conf
							try {
								fs_watcher.remove_watch(cdir/name/arguments.confname);
							} catch (const std::invalid_argument &) {
								mcshub::log->debug("already removed \"" + name + "\" conf or distribution is disabled");
							}
							// erase mcsman conf if persists
							new_conf->servers.erase(name);
						}
					}
				}
			} else {
				// sub conf
				std::string name = *(--(--event.watch.path().end()));
				if (event.mask & inev::delete_self) {
					// sub conf: delete_self
					new_conf->servers.erase(name);
					mcshub::log->verbose("conf for \"" + name + "\" was deleted");
					if (arguments.mcsman) {
						new_conf->servers[name] = config::server_record {
							name, arguments.default_port, arguments.status, arguments.login, main_log, false,
							{ { "name", name } }
						};
						mcshub::log->verbose("but mcsman conf for \"" + name + "\" was recreated");
					}
				}
				if (event.mask & inev::close_write) {
					// sub conf: close_write
					auto node = YAML::LoadFile(event.watch.path());
					auto & servers = new_conf->servers;
					auto iter = servers.find(name);
					if (iter != servers.end()) {
						node >> iter->second;
					} else {
						config::server_record record = conf_record_default();
						node >> record;
						servers[name] = record;
					}
					mcshub::log->verbose("reload conf for \"" + name + "\"");
				}
			}
		}
		conf_instance = new_conf;
	});
}

log_level str2lvl(const std::string & verb) {
	if (verb == "none")
		return log_level::none;
	if (verb == "fatal")
		return log_level::fatal;
	if (verb == "error")
		return log_level::error;
	if (verb == "warning")
		return log_level::warning;
	if (verb == "info")
		return log_level::info;
	if (verb == "verbose")
		return log_level::verbose;
	if (verb == "debug")
		return log_level::debug;
	throw config_exception("verb", "no '" + verb + "' log level");
}

void operator>>(const YAML::Node & node, config::server_record & record) {
	if (auto address = node["address"])
		record.address = address.as<std::string>();
	if (auto port = node["port"])
		record.port = port.as<std::uint16_t>();
	if (auto status = node["status"])
		record.status = status.as<std::string>();
	if (auto login = node["login"])
		record.login = login.as<std::string>();
	if (auto log = node["log"])
		record.log = log.as<std::string>();
	for (auto item : node["vars"]) {
		record.vars[item.first.as<std::string>()] = item.second.as<std::string>();
	}
}

void operator>>(const YAML::Node & node, config::dns_module & dns) {
	if (auto use_dns = node["use_dns"])
		dns.use_dns = use_dns.as<bool>();
	if (auto ttl = node["ttl"])
		dns.ttl = ttl.as<std::uint32_t>();
	if (auto records = node["records"]) {
		for (auto record : records)
			dns.records.push_back(dns_packet::parse_dns_record(record.as<std::string>()));
	}
}

void operator>>(const YAML::Node & node, config & conf) {
	if (auto address = node["address"])
		conf.address = address.as<std::string>();
	if (auto port = node["port"])
		conf.port = port.as<std::uint16_t>();
	if (auto threads = node["threads"])
		conf.threads = threads.as<unsigned int>();
	if (auto log = node["log"])
		conf.log = log.as<std::string>();
	if (auto verb = node["verb"]) {
		try {
			conf.verb = (log_level)verb.as<int>();
		} catch (YAML::Exception &) {
			conf.verb = str2lvl(verb.as<std::string>());
		}
	}
	if (auto distributed = node["distributed"])
		conf.distributed = distributed.as<bool>();
	if (auto dns = node["dns"])
		dns >> conf.dns;
	if (auto default_server = node["default"]) {
		if (!default_server.IsMap())
			throw config_exception("default", "not a map yaml structure");
		default_server >> conf.default_server;
	}
	if (auto servers = node["servers"]) {
		if (!servers.IsMap())
			throw config_exception("servers", "not a map yaml structure");
		for (auto record : servers) {
			config::server_record server;
			record.second >> server;
			conf.servers[record.first.as<std::string>()] = server;
		}
	}
}

void operator<<(YAML::Node & node, const config::server_record & record) {
	node["address"] = record.address;
	node["port"] = record.port;
	node["status"] = record.status;
	node["login"] = record.login;
	node["log"] = std::string(record.log);

	auto vars = node["vars"];
	for (auto var : record.vars)
		vars[var.first] = var.second;
}

void operator<<(YAML::Node & node, const config & conf) {
	node["address"] = conf.address;
	node["port"] = conf.port;
	node["threads"] = conf.threads;
	node["log"] = std::string(conf.log);
	node["verb"] = log_lvl2str(conf.verb);
	auto default_server = node["default"];
	default_server << conf.default_server;

	auto servers = node["servers"];
	for (auto server : conf.servers) {
		auto record = node[server.first];
		record << server.second;
	}
}

void config::load(const std::string & path) {
	std::ifstream file(path);
	if (!file) {
		file.close();
		throw std::runtime_error("cannot load configuration from file '" + std::string(path) + "'");
	}
	auto node = YAML::Load(file);
	node >> *this;
}

void config::install() const {
	if (!fs::exists(arguments.default_srv_dir) && !fs::create_directory(arguments.default_srv_dir))
		throw std::runtime_error("can't create directory");
	std::ofstream status_file(arguments.default_srv_dir + '/' + arguments.status);
	status_file << file_content::default_status;
	status_file.close();
	std::ofstream login_file(arguments.default_srv_dir + '/' + arguments.login);
	login_file << file_content::default_login;
	login_file.close();
	std::ofstream config_file(arguments.confname);
	YAML::Node node;
	node << *this;
	config_file << "# MCSHub configuration file" << std::endl << node;
	config_file.close();
}

void config::static_install() {
	auto c = conf_install();
	c->install();
}

} // mcshub
