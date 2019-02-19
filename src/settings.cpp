#include "settings.h"

#include <fstream>
#include <yaml-cpp/yaml.h>
#include <experimental/filesystem>

#include "hardcoded.h"

namespace fs = std::experimental::filesystem;

namespace mcshub {

config conf {
	"0.0.0.0", 25565, 1, "$std", log_level::info,
	{ "", 25565, "default/status.json", "default/login.json", "$main" }
};

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
	node["log"] = record.log;

	auto vars = node["vars"];
	for (auto var : record.vars)
		vars[var.first] = var.second;
}

void operator<<(YAML::Node & node, const config & conf) {
	node["address"] = conf.address;
	node["port"] = conf.port;
	node["threads"] = conf.threads;
	node["log"] = conf.log;
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
		throw std::runtime_error("cannot load configuration from file '" + path + "'");
	}
	auto node = YAML::Load(file);
	node >> conf;
}

void config::install() {
	if (!fs::create_directory("default"))
		throw std::runtime_error("can't create directory");
	std::ofstream status_file("default/status.json");
	status_file << file_content::default_status;
	status_file.close();
	std::ofstream login_file("default/login.json");
	login_file << file_content::default_login;
	login_file.close();
	std::ofstream config_file("config.yml");
	YAML::Node node;
	node << *this;
	config_file << "# MCSHub configuration file" << std::endl << node;
	config_file.close();
}

} // mcshub
