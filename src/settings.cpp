#include "settings.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <unordered_set>
#include <algorithm>

#include "hardcoded.h"
#include "log.h"
#include "prog_args.h"
#include "inotify_d.h"
#include "dns_server.h"

namespace fs = std::filesystem;

namespace mcshub {

const char * std_log = "$std";
const char * main_log = "$main";

const fs::path cdir = ".";

inotify_d fs_watcher;

void operator>>(const YAML::Node & node, config::server_record & record);
void operator>>(const YAML::Node & node, config::dns_module & dns);
void operator>>(const YAML::Node & node, config & conf);
void operator<<(YAML::Node & node, const config::server_record & record);
void operator<<(YAML::Node & node, const config & conf);

enum class file_category {
	main_conf, main_dir, srv_conf, srv_dir
};

struct watch_data : public watch_t::data_t {
	virtual file_category category() const noexcept = 0;
};
struct main_conf_wt : public watch_data {
	virtual file_category category() const noexcept {
		return file_category::main_conf;
	} 
} main_conf;
struct main_dir_wt : public watch_data {
	virtual file_category category() const noexcept {
		return file_category::main_dir;
	} 
} main_dir;
struct srv_conf_wt : public watch_data {
	virtual file_category category() const noexcept {
		return file_category::srv_conf;
	} 
} srv_conf;
struct srv_dir_wt : public watch_data {
	virtual file_category category() const noexcept {
		return file_category::srv_dir;
	} 
} srv_dir;

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
	fs_watcher.add_watch(inev::close_write | inev::delete_self, arguments.confname, &main_conf);
	fs_watcher.add_watch(inev::create | inev::in_delete, cdir, &main_dir);
	// load configurations for all sub directories
	for (const auto & file : fs::directory_iterator(cdir)) {
		if (file.is_directory()) {
			auto & servers = c->servers;
			std::string name = file.path().filename();
			if (arguments.mcsman) {
				// load mcsman settings first
				log_info("init mcsman configuration for \"" + name + "\"");
				servers[name] = config::server_record {
					name, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login, main_log, false,
					{ { "name", name } }
				};
			}
			fs_watcher.add_watch(inev::create | inev::delete_self, file.path(), &srv_dir);
			fs::path conf = file.path()/arguments.confname;
			if (fs::exists(conf) && fs::is_regular_file(conf)) {
				// load sub conf
				if (c->distributed) {
					log_info("init distributed configuration for \"" + name + "\"");
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
				fs_watcher.add_watch(inev::close_write | inev::delete_self, conf, &srv_conf);
			}
		}
	}
}

void config::init_listener(event & poll) {
	poll.add(fs_watcher, [](descriptor &, std::uint32_t) {
		std::vector<inotify_d::event_t> events = fs_watcher.read();
		std::shared_ptr<const config> old_conf = conf;
		std::shared_ptr<config> new_conf = std::make_shared<config>(*old_conf);
		for (const inotify_d::event_t & event : events) {
			// 1: main_conf { close_write, delete_self }
			// 2: main_dir { create(srv_dir, main_conf), delete }
			// 3: srv_conf { delete_self, close_write }
			// 4: srv_dir { create(srv_conf), delete_self }
			if (event.watch.data == &main_conf) {
				// 1: main_conf
				if (event.mask & inev::close_write) {
					// main_conf: close_write
					try {
						auto node = YAML::LoadFile(arguments.confname);
						node >> *new_conf;
					} catch (const YAML::Exception & yaml_e) {
						log_error("main configuration file has problems");
						log_error(yaml_e);
					}
					log_info("reloaded main configuration");
				}
				if (event.mask & inev::delete_self) {
					// main conf: delete_self
					log_error("main config file was deleted");
					fs_watcher.remove_watch(event.watch);
				}
			} else if (event.watch.data == &main_dir) {
				// 2: main_dir
				if (event.mask & inev::create) {
					// main_dir: create
					const std::string & name = event.subject;
					if (fs::is_directory(name)) {
						// create(srv_dir)
						if (arguments.mcsman) {
							// add mcsman auto-record
							log_info("added new mcsman server configuration \"" + name + "\"");
							new_conf->servers[name] = config::server_record {
								name, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login, main_log, false,
								{ { "name", name } }
							};
						}
						fs_watcher.add_watch(inev::delete_self | inev::create, cdir/name, &srv_dir);
					}
					if (name == arguments.confname) {
						// create(main_conf)
						try {
							auto node = YAML::LoadFile(arguments.confname);
							node >> *new_conf;
						} catch (const YAML::Exception & yaml_e) {
							log_error("main configuration file \"" + name + "\" has problems");
							log_error(yaml_e);
						}
						fs_watcher.add_watch(inev::delete_self | inev::close_write, arguments.confname, &main_conf);
						log_info("main configuration created again after destroying");
					}
				}
				if (event.mask & inev::in_delete) {
					// main_dir: delete
				}
			} else if (event.watch.data == &srv_conf) {
				// 3: srv_conf
				std::string name = *(--(--event.watch.path().end()));
				if (event.mask & inev::delete_self) {
					// srv_conf: delete_self
					new_conf->servers.erase(name);
					if (old_conf->distributed) {
						log_verbose("conf for \"" + name + "\" was deleted");
						if (arguments.mcsman) {
							new_conf->servers[name] = config::server_record {
								name, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login, main_log, false,
								{ { "name", name } }
							};
							log_verbose("but mcsman conf for \"" + name + "\" was recreated");
						}
					}
					fs_watcher.remove_watch(event.watch);
				}
				if (event.mask & inev::close_write && new_conf->distributed) {
					// srv_conf: close_write
					try {
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
						log_verbose("reload conf for \"" + name + "\"");
					} catch (const YAML::Exception & yaml_e) {
						log_error("configuration file for server \"" + name + "\" has problems");
						log_error(yaml_e);
					}
				}
			} else if (event.watch.data == &srv_dir) {
				// 4: srv_dir
				if (event.mask & inev::create) {
					// srv_dir: create
					const fs::path & path = event.watch.path();
					std::string name = path.filename();
					fs::path conf_file = cdir/name/arguments.confname;
					if (arguments.confname == event.subject) {
						// create(srv_conf)
						if (new_conf->distributed) {
							try {
								auto node = YAML::LoadFile(conf_file);
								auto & servers = new_conf->servers;
								auto iter = servers.find(name);
								if (iter != servers.end()) {
									node >> iter->second;
								} else {
									config::server_record record = conf_record_default();
									node >> record;
									servers[name] = record;
								}
							} catch (const YAML::Exception & yaml_e) {
								log_error("configuration file \"" + std::string(conf_file) +
									"\" for server \"" + name + "\" has problems");
								log_error(yaml_e);
							}
							log_verbose("created configuration for \"" + name + "\"");
						}
						fs_watcher.add_watch(inev::delete_self | inev::close_write, conf_file, &srv_conf);
					}
				}
				if (event.mask & inev::delete_self) {
					// srv_dir: delete_self
					if (arguments.mcsman) {
						// erase mcsman conf if persists
						std::string name = event.watch.path().filename();
						log_info("mcsman configuration for \"" + name + "\" was deleted");
						new_conf->servers.erase(name);
					}
					fs_watcher.remove_watch(event.watch);
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
	if (auto allowFML = node["fml"])
		record.allowFML = allowFML.as<bool>();
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
	node["log"] = (const std::string &)(record.log);

	auto vars = node["vars"];
	for (auto var : record.vars)
		vars[var.first] = var.second;
}

void operator<<(YAML::Node & node, const config & conf) {
	node["address"] = (const std::string &)(conf.address);
	node["port"] = std::uint16_t(conf.port);
	node["threads"] = unsigned(conf.threads);
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
