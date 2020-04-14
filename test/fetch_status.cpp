#include <thread>

#include "config.hpp"

#include "test_server.hpp"
#include "test.hpp"

#define TEST_STRING "description: testificate as testers"
#define STATUS_FILE "status.yml"

test {
	using namespace mcshub;
	using namespace std::chrono_literals;
	using namespace std::string_literals;
	mcshub::mcshub server(ekutils::stream::in | ekutils::stream::err);
	mcshub::sclient client = server.client();
	pakets::response res = client.status("localhost", server.port());
	std::cout << res.message() << std::endl;

	server.dir()->mk_file(STATUS_FILE) << TEST_STRING;
	log_debug("WORKING DIRECTORY: " + server.dir()->path.string());
	server.input.writeln("conf scope default record auto set name=test, status=\"!file /" STATUS_FILE "\"");
	server.input.writeln("conf scope default domain mc.handtruth.com");
	server.input.writeln("conf reload");
	server.input.writeln("ping");
	std::string pong;
	int wait_cnt = 0;
	while (!server.errors.readln(pong)) {
		wait_cnt++;
	}
	assert_true(wait_cnt >= 0 && wait_cnt < 2);
	assert_equals("pong", pong);

	client.reconnect();
	res = client.status("test.mc.handtruth.com", server.port());
	assert_equals(R"===({"description":"testificate as testers","version":{"name":")===" + config::project + "-" + config::version + R"===(","protocol":-1},"players":{"sample":[],"max":20,"online":0}})===", res.message());
}
