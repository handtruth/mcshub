#ifdef TEST_SERVER_HEAD
#   error "test_server.hpp header can't be included several times"
#else
#   define TEST_SERVER_HEAD
#endif

// LCOV_EXCL_START

#include <unistd.h>
#include <sys/mount.h>
#include <filesystem>
#include <fstream>
#include <forward_list>

#include <yaml-cpp/yaml.h>

#include <ekutils/process.hpp>
#include <ekutils/reader.hpp>
#include <ekutils/writer.hpp>
#include <ekutils/event_d.hpp>
#include <ekutils/log.hpp>
#include <ekutils/idgen.hpp>

#include "settings.hpp"
#include "mcshub_arguments.hpp"
#include "mcshub.hpp"
#include "sclient.hpp"
#include "thread_controller.hpp"

namespace fs = std::filesystem;

namespace mcshub {

arguments_t test_arguments() {
	arguments_t args = arguments;
	args.threads = 1;
	args.verb = ekutils::log_level::debug;
	args.address = "tcp://localhost:0";
	args.cli = true;
	return args;
}

#define insert(node, name, record) \
		(node[#name] = record.name)
#define insert_str(node, name, record) \
		if (!record.name.empty())      \
			insert(node, name, record)
#define insert_int(node, name, record) \
		if (record.name)               \
			insert(node, name, record)
#define insert_bool(node, name, record, def_val) \
		if (record.name != def_val)              \
			insert(node, name, record)

YAML::Node base_record2yaml(const settings::basic_record & record) {
	if (record.drop)
		return YAML::Node("drop");
	YAML::Node node;
	insert_str(node, address, record);
	if (!record.login.empty())
		node["login"] = record.login.src();
	if (!record.status.empty())
		node["status"] = record.status.src();
	if (!record.vars.empty()) {
		YAML::Node vars;
		for (auto & each : record.vars) {
			vars[each.first] = each.second;
		}
		node["vars"] = vars;
	}
	return node;
}

YAML::Node srv_record2yaml(const settings::server_record & record) {
	YAML::Node node = base_record2yaml(record);
	if (record.drop)
		return node;
	if (record.fml.has_value())
		node["fml"] = base_record2yaml(record.fml.value());
	return node;
}

YAML::Node settings2yaml(const settings & config) {
	YAML::Node node;
	insert_str(node, address, config);
	insert_bool(node, distributed, config, false);
	auto def_serv = srv_record2yaml(config.default_server);
	if (def_serv.size() != 0)
		node["default"] = def_serv;
	insert_bool(node, dns_cache, config, true);
	insert_str(node, domain, config);
	insert_str(node, log, config);
	insert_int(node, max_packet_size, config);
	insert_int(node, threads, config);
	insert_int(node, timeout, config);
	if (config.verb != ekutils::log_level::none)
		node["verb"] = ekutils::log_lvl2str(config.verb);
	if (!config.servers.empty()) {
		YAML::Node servers;
		for (auto & pair : config.servers)
			servers[pair.first] = srv_record2yaml(pair.second);
		node["servers"] = servers;
	}
	return node;
}

#undef insert
#undef insert_str
#undef insert_int
#undef insert_bool

class confset {
	static ekutils::idgen<unsigned> ids;

	confset() {
		if (fs::exists(path)) {
			fs::remove_all(path);
			umount(path.c_str());
		}
		log_debug("creating test mcshub working directory: \"" + path.string() + '"');
		fs::create_directory(path);
		mount("tmpfs", path.c_str(), "tmpfs", MS_NOATIME, nullptr);
	}
public:
	const fs::path path = fs::weakly_canonical("test_" + std::to_string(getpid()) + "-" + std::to_string(ids.next()));
	static std::shared_ptr<confset> create() {
		std::shared_ptr<confset> result;
		result.reset(new confset());
		return result;
	}
	~confset() {
		umount(path.c_str());
		fs::remove_all(path);
	}
	void mk_server(const std::string & name) {
		fs::create_directory(path/name);
	}
	void mk_server(const std::string & name, const YAML::Node & node) {
		mk_server(name);
		std::ofstream file(path/name/"mcshub.yml");
		file << node;
	}
	void mk_server(const std::string & name, const settings::server_record & conf) {
		mk_server(name, srv_record2yaml(conf));
	}
	void mk_config(const settings & conf) {
		std::ofstream file(path/"mcshub.yml");
		file << settings2yaml(conf);
	}
	std::ofstream mk_file(const std::string & filename) {
		return std::ofstream(path/filename);
	}
	std::ofstream mk_file(const std::string & name, const std::string & filename) {
		mk_server(name);
		return std::ofstream(path/name/filename);
	}
};

ekutils::idgen<unsigned> confset::ids;

class mcshub : public ekutils::process {
	std::uint16_t portn = 0;
	std::shared_ptr<confset> confdir;
public:
	ekutils::writer  input;
	ekutils::reader output;
	ekutils::reader errors;
private:
	mcshub(const arguments_t & args, const std::shared_ptr<confset> & dir, int streams, ekutils::event_d event);
public:
	mcshub(
		const arguments_t & args = test_arguments(),
		const std::shared_ptr<confset> & conf = confset::create(),
		int streams = ekutils::stream::in
	) : mcshub(args, conf, streams, ekutils::event_d()) {}
	mcshub(int streams) : mcshub(test_arguments(), confset::create(), streams) {}
	mcshub(const std::shared_ptr<confset> & conf, int streams = ekutils::stream::in) : mcshub(test_arguments(), conf, streams) {}
	mcshub(const arguments_t & args, int streams) : mcshub(args, confset::create(), streams) {}
	~mcshub() noexcept(false) {
		if (running()) {
			kill(ekutils::sig::termination);
			int status = wait();
			if (status)
				throw std::runtime_error("mcshub service exited with error code: " + std::to_string(status));
		}
	}
	constexpr const std::shared_ptr<confset> & dir() {
		return confdir;
	}
	std::uint16_t port() const noexcept {
		return portn;
	}
	sclient client() {
		return sclient("localhost", portn);
	}
};

mcshub::mcshub(const arguments_t & args, const std::shared_ptr<confset> & dir, int streams,
		ekutils::event_d event) : ekutils::process(
[&args, &event, &dir] () -> int {
	arguments = args;
	assert(-1 != chdir(dir->path.c_str()));
	return entrypoint("mcshub-test", [&event](){
		log_info("BEGIN TESTS: portn " + std::to_string(thread_controller::real_port));
		event.write(thread_controller::real_port);
		// event.close();
	});
}, ekutils::process_opts {
	streams
}), confdir(dir), input(stdin_stream()), output(stdout_stream()), errors(stderr_stream()) {
	portn = static_cast<std::uint16_t>(event.read());
}

} // namespace mcshub

// LCOV_EXCL_STOP
