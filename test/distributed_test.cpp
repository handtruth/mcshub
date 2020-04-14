#include <iostream>

#include "test_server.hpp"
#include "test.hpp"

using namespace mcshub;

test {
	ekutils::log = new ekutils::stdout_log(ekutils::log_level::debug);
	auto args = test_arguments();
	args.distributed = true;
	args.domain = "mc.handtruth.com";
	auto conf = confset::create();
	YAML::Node test_server;
	{
		auto status = YAML::Load(R"===(
description: distributed is working
players: !ignore ignore
version: !ignore ignore
)===");
		test_server["status"] = status;
	}
	log_debug("Creating conf");
	conf->mk_server("test", test_server);
	log_debug("Server starting");
	::mcshub::mcshub server(args, conf);
	log_debug("Server started");
	sclient client = server.client();
	pakets::response resp = client.status("test.mc.handtruth.com");
	assert_equals(R"===({"description":"distributed is working"})===", resp.message());
}
