#include "manager.h"

#include <iostream>
#include <unistd.h>

#include "settings.h"
#include "signal_d.h"

namespace mcshub {

struct record_form : public cli_form {
	std::string name;
	config::basic_record record = default_record;

	static cli_form_ptr instance();
	virtual void fill(const std::map<std::string, std::string> & values) override;
};

cli_form_ptr record_form::instance() {
	return std::make_unique<record_form>();
}

void record_form::fill(const std::map<std::string, std::string> & values) {
	for (auto & entry : values) {
		if (entry.first == "address") {
			record.address = entry.second;
		} else if (entry.first == "port") {
			auto port = std::stoi(entry.second);
			if (port > std::numeric_limits<std::uint16_t>::max() || port < 0)
				throw config_exception("record.port", "port number isn't in range [0..65535]");
			record.port = static_cast<std::uint16_t>(port);
		} else if (entry.first == "status") {
			record.status = entry.second;
		} else if (entry.first == "login") {
			record.login = entry.second;
		} else if (entry.first == "name") {
			name = entry.second;
		} else {
			throw cli_form_error("option \"" + entry.first + "\" does not exists");
		}
	}
}

void special_assign(config::server_record & it, const config::basic_record & other) {
	auto fml = it.fml;
	it = other;
	it.fml = fml;
}

manager::manager() {
	root.action("stop", [](auto &) {
		kill(getpid(), sig::termination);
	}, "stop mcshub");
	root.action("help", [this](auto &) {
		for (auto & line : root.help())
			std::cerr << line << std::endl;
	}, "print help message");
	auto & conf = *root.choice("conf");
	conf.action("reload", [](auto &) {
		reload_configuration();
	}, "reload all configuration");
	auto & conf_entry = *conf.option("scope", "scope", {
		"default", "run"
	})->choice();
	conf_entry.word("domain", "domain")->action([](auto & forms) {
		const auto & scope = form_as_word(forms, "scope");
		auto domain = form_as_word(forms, "domain");
		check_domain(domain);
		if (scope == "default") {
			default_conf.domain = domain;
		} else if (scope == "run") {
			// TODO
		}
	}, "set domain name suffix to each configuration");
	conf_entry.word("timeout", "timeout")->action([](auto & forms) {
		const auto & scope = form_as_word(forms, "scope");
		unsigned long timeout = std::stoul(form_as_word(forms, "timeout"));
		if (scope == "default") {
			default_conf.timeout = timeout;
		}
	}, "set timeout");
	auto & conf_entry_record = *conf_entry.option("record", "type", {
		"default-auto", "default-fml", "auto", "fml"
	})->choice();
	conf_entry_record.form("set", "record", record_form::instance, {
		{ "name", "<server_name>" },
		{ "address", "[backend]" },
		{ "port", "[number]" },
		{ "status", "[status_response_path]" },
		{ "login", "[login_response_path]" },
	})->action([](auto & forms) {
		std::string scope = form_as_word(forms, "scope");
		if (scope == "default") {
			const auto & type = form_as_word(forms, "type");
			const auto & form = get_form<record_form>(forms, "record");
			if (type == "default-auto")
				special_assign(default_conf.default_server, form.record);
			else if (type == "default-fml")
				default_conf.default_server.fml = form.record;
			else {
				if (form.name.empty())
					throw cli_form_error("empty record name");
				auto & servers = default_conf.servers;
				auto & record = (servers.find(form.name) == servers.end()) ?
					servers.emplace(form.name, default_record).first->second :
					servers[form.name];
				if (type == "auto")
					special_assign(record, form.record);
				else if (type == "fml")
					record.fml = form.record;
			}
		}
	}, "set a server record");
}

void manager::on_line() {
	std::string line;
	while (input.read_line(line)) {
		try {
			std::unordered_map<std::string, cli_form_ptr> forms;
			root.process(line.c_str(), forms);
		} catch (const std::exception & e) {
			std::cerr << e.what() << std::endl;
		}
		line.clear();
	}
}

}
