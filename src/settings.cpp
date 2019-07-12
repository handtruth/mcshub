#include "settings.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <unordered_set>
#include <algorithm>

#include <sys/sysinfo.h>

#include "resources.h"
#include "log.h"
#include "prog_args.h"
#include "inotify_d.h"
#include "dns_server.h"

namespace fs = std::filesystem;

namespace mcshub {

const char * std_log = "$std";

const fs::path cdir = ".";

inotify_d fs_watcher;

void operator>>(const YAML::Node & node, config::server_record & record);
void operator>>(const YAML::Node & node, config::dns_module & dns);
void operator>>(const YAML::Node & node, config & conf);

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
	return { "", arguments.default_port, "", "", true, false };
}

std::shared_ptr<config> conf_default() {
	std::shared_ptr<config> ptr = std::make_shared<config>(config {
		"0.0.0.0", arguments.port, unsigned(get_nprocs()), 6000, std_log, log_level::info, false, { false }, std::string(),
		conf_record_default()
	});
	return ptr;
}

std::shared_ptr<config> conf_install() {
	std::shared_ptr<config> ptr = std::make_shared<config>(config {
		"0.0.0.0", arguments.port, 1, 6000, std_log, log_level::info, false, { false }, { std::string() },
		{ "", arguments.default_port, cdir/arguments.default_srv_dir/arguments.status,
			cdir/arguments.default_srv_dir/arguments.login, true }
	});
	return ptr;
}

matomic<std::shared_ptr<const config>> conf_instance;
const matomic<std::shared_ptr<const config>> & conf = conf_instance;

void load_all_conf(const std::shared_ptr<config> & c, bool add_watch = false) {
	c->load(arguments.confname);
	// load configurations for all sub directories
	for (const auto & file : fs::directory_iterator(cdir)) {
		if (file.is_directory()) {
			auto & servers = c->servers;
			std::string name = file.path().filename();
			if (arguments.mcsman && name != "default") {
				// load mcsman settings first
				log_info("init mcsman configuration for \"" + name + "\"");
				servers[name] = config::server_record {
					name, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login, false, true,
					{ { "name", name } }
				};
			}
			if (add_watch)
				fs_watcher.add_watch(inev::create | inev::moved_to | inev::delete_self | inev::move_self, file.path(), &srv_dir);
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
				if (add_watch)
					fs_watcher.add_watch(inev::close_write | inev::delete_self | inev::move_self, conf, &srv_conf);
			}
		}
	}
}

void config::initialize() {
	fs_watcher.add_watch(inev::close_write | inev::delete_self | inev::move_self, arguments.confname, &main_conf);
	fs_watcher.add_watch(inev::create | inev::moved_to | inev::in_delete, cdir, &main_dir);
	auto c = conf_default();
	load_all_conf(c, true);
	conf_instance = c;
}

void config::init_listener(event & poll) {
	poll.add(fs_watcher, [](descriptor &, std::uint32_t) {
		std::vector<inotify_d::event_t> events = fs_watcher.read();
		std::shared_ptr<const config> old_conf = conf;
		std::shared_ptr<config> new_conf = std::make_shared<config>(*old_conf);
		for (auto iter = events.rbegin(); iter != events.rend(); iter++) {
			auto event = *iter;
			// 1: main_conf { close_write, delete_self | move_self }
			// 2: main_dir { create | moved_to (srv_dir, main_conf), delete | moved_from }
			// 3: srv_conf { delete_self | move_self, close_write }
			// 4: srv_dir { create | moved_to (srv_conf), delete_self | move_self }
			if (event.watch.data == &main_conf) {
				// 1: main_conf
				if (event.mask & inev::close_write) {
					// main_conf: close_write
					try {
						// Unfortunately, it is required to reload everything :(
						new_conf = conf_default();
						load_all_conf(new_conf);
					} catch (const YAML::Exception & yaml_e) {
						log_error("main configuration file has problems");
						log_error(yaml_e);
					}
					log_info("reloaded main configuration");
				}
				if (event.mask & inev::delete_self || event.mask & inev::move_self) {
					// main conf: delete_self
					log_error("main config file was deleted");
					fs_watcher.remove_watch(event.watch);
				}
			} else if (event.watch.data == &main_dir) {
				// 2: main_dir
				if (event.mask & inev::create || event.mask & inev::moved_to) {
					// main_dir: create
					const std::string & name = event.subject;
					if (fs::is_directory(name)) {
						// create | moved_to (srv_dir)
						if (arguments.mcsman && name != "default") {
							// add mcsman auto-record
							log_info("added new mcsman server configuration \"" + name + "\"");
							new_conf->servers[name] = config::server_record {
								name, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login, false, true,
								{ { "name", name } }
							};
						}
						fs_watcher.add_watch(inev::delete_self | inev::move_self | inev::create | inev::moved_to, cdir/name, &srv_dir);
					}
					if (name == arguments.confname) {
						// create | moved_to (main_conf)
						try {
							auto node = YAML::LoadFile(arguments.confname);
							node >> *new_conf;
						} catch (const YAML::Exception & yaml_e) {
							log_error("main configuration file \"" + name + "\" has problems");
							log_error(yaml_e);
						}
						fs_watcher.add_watch(inev::delete_self | inev::move_self | inev::close_write, arguments.confname, &main_conf);
						log_info("main configuration created again after destroying");
					}
				}
				if (event.mask & inev::in_delete || event.mask & inev::moved_from) {
					// main_dir: delete
				}
			} else if (event.watch.data == &srv_conf) {
				// 3: srv_conf
				std::string name = *(--(--event.watch.path().end()));
				if (event.mask & inev::delete_self || event.mask & inev::move_self) {
					// srv_conf: delete_self | move_self
					new_conf->servers.erase(name);
					if (old_conf->distributed) {
						log_verbose("conf for \"" + name + "\" was deleted");
						if (arguments.mcsman && name != "default") {
							new_conf->servers[name] = config::server_record {
								name, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login, false, true,
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
				if (event.mask & inev::create || event.mask & inev::moved_to) {
					// srv_dir: create | moved_to
					const fs::path & path = event.watch.path();
					std::string name = path.filename();
					fs::path conf_file = cdir/name/arguments.confname;
					if (arguments.confname == event.subject) {
						// create | moved_to(srv_conf)
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
						fs_watcher.add_watch(inev::delete_self | inev::move_self | inev::close_write, conf_file, &srv_conf);
					}
				}
				if (event.mask & inev::delete_self || event.mask & inev::move_self) {
					// srv_dir: delete_self | move_self
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
	if (auto domain = node["domain"]) {
		std::string & name = conf.domain = domain.as<std::string>();
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
				name = '.' + name;
			}
		}
	}
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
	if (!fs::exists(arguments.confname)) {
		std::ofstream status_file(arguments.default_srv_dir + '/' + arguments.status);
		status_file.write(reinterpret_cast<const char *>(res::sample_default_status_json), res::sample_default_status_json_len);
		status_file.close();
		std::ofstream login_file(arguments.default_srv_dir + '/' + arguments.login);
		login_file.write(reinterpret_cast<const char *>(res::sample_default_login_json), res::sample_default_login_json_len);
		login_file.close();
		std::ofstream config_file(arguments.confname);
		config_file.write(reinterpret_cast<const char *>(res::sample_mcshub_yml), res::sample_mcshub_yml_len);
		config_file.close();
	}
}

void config::static_install() {
	auto c = conf_install();
	c->install();
}

} // mcshub
