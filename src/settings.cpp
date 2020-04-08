#include "settings.hpp"

#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <sys/sysinfo.h>

#include <yaml-cpp/yaml.h>
#include <ekutils/inotify_d.hpp>
#include <ekutils/primitives.hpp>
#include <ekutils/log.hpp>

#include "resources.hpp"
#include "prog_args.hpp"

namespace fs = std::filesystem;

namespace mcshub {

using ekutils::byte_t;

const char * std_log = "$std";

const fs::path cdir = ".";

ekutils::inotify_d fs_watcher;

void fill_record(const YAML::Node & node, settings::basic_record & record);
void operator>>(const YAML::Node & node, settings::server_record & record);
void operator>>(const YAML::Node & node, settings & conf);

void put_main_conf_file();

enum class file_category {
	main_conf, main_dir, srv_conf, srv_dir
};

struct watch_data : public ekutils::watch_t::data_t {
	virtual file_category category() const noexcept = 0;
};
struct main_conf_wt : public watch_data {
	virtual file_category category() const noexcept override {
		return file_category::main_conf;
	} 
} main_conf;
struct main_dir_wt : public watch_data {
	virtual file_category category() const noexcept override {
		return file_category::main_dir;
	} 
} main_dir;
struct srv_conf_wt : public watch_data {
	virtual file_category category() const noexcept override {
		return file_category::srv_conf;
	} 
} srv_conf;
struct srv_dir_wt : public watch_data {
	virtual file_category category() const noexcept override {
		return file_category::srv_dir;
	} 
} srv_dir;

inline settings::server_record conf_record_mcsman(const std::string & name) {
	const std::string domain = name + "-mcs";
	return {
		domain, arguments.default_port, cdir/name/arguments.status, cdir/name/arguments.login,
		false, true, { { "name", name } }
	};
}

settings default_conf;
settings::basic_record default_record;

ekutils::matomic<std::shared_ptr<const settings>> conf_instance;
const ekutils::matomic<std::shared_ptr<const settings>> & conf = conf_instance;

void load_all_conf(const std::shared_ptr<settings> & c, bool add_watch = false) {
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
			if (add_watch) {
				using namespace ekutils::inev;
				fs_watcher.add_watch(create | moved_to | delete_self | move_self, file.path(), &srv_dir);
			}
			fs::path conf_f = file.path()/arguments.confname;
			if (fs::exists(conf_f) && fs::is_regular_file(conf_f)) {
				// load sub conf
				if (c->distributed) {
					log_info("init distributed configuration for \"" + name + "\"");
					auto node = YAML::LoadFile(conf_f);
					auto iter = servers.find(name);
					if (iter != servers.end()) {
						node >> iter->second;
					} else {
						settings::server_record record = default_record;
						node >> record;
						servers[name] = record;
					}
				}
				if (add_watch) {
					using namespace ekutils::inev;
					fs_watcher.add_watch(close_write | delete_self | move_self, conf_f, &srv_conf);
				}
			}
		}
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
		arguments.port, // port
		unsigned(get_nprocs()), // threads
		arguments.max_packet_size, // max_packet_size
		arguments.timeout, // timeout
		std_log, // log
		ekutils::log_level::info, // verb
		arguments.distributed, // distributed
		arguments.domain, // domain
		{ // default
			"", // address
			arguments.default_port, // port
			cdir/arguments.default_srv_dir/arguments.status, // status
			cdir/arguments.default_srv_dir/arguments.login, // login
			arguments.drop, // drop
			false, // mcsman
			{}, // vars
		},
		{}, // servers
		!arguments.no_dns_cache // dns_cache
	};
	default_record = {
		std::string(), //address
		arguments.default_port, // port
		"", // status
		"", // login
		false, // drop
		false, // mcsman
		{} //vars
	};
	using namespace ekutils::inev;
	fs_watcher.add_watch(close_write | delete_self | move_self, arguments.confname, &main_conf);
	fs_watcher.add_watch(create | moved_to | in_delete, cdir, &main_dir);
	auto c = std::make_shared<settings>(default_conf);
	load_all_conf(c, true);
	conf_instance = c;
}

void settings::init_listener(ekutils::epoll_d & poll) {
	poll.add(fs_watcher, [](auto &, auto) {
		std::vector<ekutils::inotify_d::event_t> events = fs_watcher.read();
		std::shared_ptr<const settings> old_conf = conf;
		std::shared_ptr<settings> new_conf = std::make_shared<settings>(*old_conf);
		for (auto iter = events.rbegin(); iter != events.rend(); iter++) {
			using ekutils::inev::inev_t;
			auto event = *iter;
			// 1: main_conf { close_write, delete_self | move_self }
			// 2: main_dir { create | moved_to (srv_dir, main_conf), delete | moved_from }
			// 3: srv_conf { delete_self | move_self, close_write }
			// 4: srv_dir { create | moved_to (srv_conf), delete_self | move_self }
			if (event.watch.data == &main_conf) {
				// 1: main_conf
				if (event.mask & inev_t::close_write) {
					// main_conf: close_write
					try {
						// Unfortunately, it is required to reload everything :(
						new_conf = std::make_shared<settings>(default_conf);
						load_all_conf(new_conf);
					} catch (const YAML::Exception & yaml_e) {
						log_error("main configuration file has problems");
						log_error(yaml_e);
					}
					log_info("reloaded main configuration");
				}
				if (event.mask & inev_t::delete_self || event.mask & inev_t::move_self) {
					// main conf: delete_self
					log_error("main settings file was deleted");
					fs_watcher.remove_watch(event.watch);
				}
			} else if (event.watch.data == &main_dir) {
				// 2: main_dir
				if (event.mask & inev_t::create || event.mask & inev_t::moved_to) {
					// main_dir: create
					const std::string & name = event.subject;
					if (fs::is_directory(name)) {
						// create | moved_to (srv_dir)
						if (arguments.mcsman && name != "default") {
							// add mcsman auto-record
							log_info("added new mcsman server configuration \"" + name + "\"");
							new_conf->servers[name] = conf_record_mcsman(name);
						}
						using namespace ekutils::inev;
						fs_watcher.add_watch(delete_self | move_self | create | moved_to, cdir/name, &srv_dir);
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
						using namespace ekutils::inev;
						fs_watcher.add_watch(delete_self | move_self | close_write, arguments.confname, &main_conf);
						log_info("main configuration created again after destroying");
					}
				}
				if (event.mask & inev_t::in_delete || event.mask & inev_t::moved_from) {
					// main_dir: delete
				}
			} else if (event.watch.data == &srv_conf) {
				// 3: srv_conf
				std::string name = *(--(--event.watch.path().end()));
				if (event.mask & inev_t::delete_self || event.mask & inev_t::move_self) {
					// srv_conf: delete_self | move_self
					new_conf->servers.erase(name);
					if (old_conf->distributed) {
						log_verbose("conf for \"" + name + "\" was deleted");
						if (arguments.mcsman && name != "default") {
							new_conf->servers[name] = conf_record_mcsman(name);
							log_verbose("but mcsman conf for \"" + name + "\" was recreated");
						}
					}
					fs_watcher.remove_watch(event.watch);
				}
				if (event.mask & inev_t::close_write && new_conf->distributed) {
					// srv_conf: close_write
					try {
						auto node = YAML::LoadFile(event.watch.path());
						auto & servers = new_conf->servers;
						auto server = servers.find(name);
						if (server != servers.end()) {
							node >> server->second;
						} else {
							settings::server_record record = default_record;
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
				if (event.mask & inev_t::create || event.mask & inev_t::moved_to) {
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
								auto server = servers.find(name);
								if (server != servers.end()) {
									node >> server->second;
								} else {
									settings::server_record record = default_record;
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
						using namespace ekutils::inev;
						fs_watcher.add_watch(delete_self | move_self | close_write, conf_file, &srv_conf);
					}
				}
				if (event.mask & inev_t::delete_self || event.mask & inev_t::move_self) {
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

void fill_record(const YAML::Node & node, settings::basic_record & record) {
	if (node.IsScalar()) {
		if (node.as<std::string>() == "drop") {
			record.drop = true;
			return;
		}
		throw config_exception("record", "this field should equals to \"drop\" or server record object");
	} else
		record.drop = false;
	if (auto address = node["address"])
		record.address = address.as<std::string>();
	if (auto port = node["port"])
		record.port = port.as<std::uint16_t>();
	else
		record.port = arguments.default_port;
	if (auto status = node["status"])
		record.status = status.as<std::string>();
	if (auto login = node["login"])
		record.login = login.as<std::string>();
	for (auto item : node["vars"]) {
		record.vars[item.first.as<std::string>()] = item.second.as<std::string>();
	}
}

void operator>>(const YAML::Node & node, settings::server_record & record) {
	fill_record(node, record);
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
			fill_record(fml, record.fml.emplace(default_record));
		}
	}
}

void operator>>(const YAML::Node & node, settings & conf) {
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
		default_server >> conf.default_server;
	}
	if (auto servers = node["servers"]) {
		if (!servers.IsMap())
			throw config_exception("servers", "not a map yaml structure");
		for (auto record : servers) {
			settings::server_record server = default_record;
			record.second >> server;
			conf.servers[record.first.as<std::string>()] = server;
		}
	}
	if (auto dns_cache = node["dns_cache"])
		conf.dns_cache = dns_cache.as<bool>();
}

void settings::load(const std::string & path) {
	auto node = YAML::LoadFile(path);
	node >> *this;
}

void settings::load(std::istream & input) {
	auto node = YAML::Load(input);
	//YAML::Node node;
	node >> *this;
}

template <std::size_t N>
std::ostream & operator<<(std::ofstream & output, const std::array<byte_t, N> & data) {
	output.write(reinterpret_cast<const char *>(data.data()), data.size());
	return output;
}

void put_main_conf_file() {
	std::ofstream config_file(arguments.confname);
	config_file << res::config::mcshub_yml;
	config_file.close();
}

void settings::static_install() {
	if (!fs::exists(arguments.default_srv_dir) && !fs::create_directory(arguments.default_srv_dir))
		throw std::runtime_error("can't create directory");
	std::ofstream status_file(arguments.default_srv_dir + '/' + arguments.status);
	status_file << res::config::fallback::status_json;
	status_file.close();
	std::ofstream login_file(arguments.default_srv_dir + '/' + arguments.login);
	login_file << res::config::fallback::login_json;
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
			name = '.' + name;
		}
	}
}

} // mcshub
