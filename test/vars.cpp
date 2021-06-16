#include <filesystem>
#include <fstream>

#include <ekutils/uuid.hpp>
#include <ekutils/log.hpp>

#include "config.hpp"
#include "response_props.hpp"
#include "status_builder.hpp"

#include "test.hpp"

const std::string & context = "there_slcow";

#define TEXT_IMG "text: not image"

test {
	using namespace mcshub;
	using namespace std::string_literals;
	pakets::handshake hs;
	hs.version() = 959;
	hs.port() = 3566;
	hs.address() = "example.com";
	std::unordered_map<std::string, std::string> pairs;
	pairs["name"] = "TESTIFICATE";
	img_vars img;
	server_vars svars;
	svars.vars = &pairs;
	file_vars fvars;
	fvars.srv_name = context;
	
	img.srv_name = context;
	auto vars = make_vars_manager(main_vars, img, svars, fvars);
	std::filesystem::create_directory(context);
	std::ofstream(context + "/icon.txt") << TEXT_IMG;
	
	{
	auto strp = build_status(YAML::Load(R"===(
description:
  - 'Server '
  - text: ${hs:address}
    color: gold
    bold: yes
  - ' is down right now'
favicon: !img icon.txt
modinfo:
  type: FML
  modList:
    - modid: ComputerCraft
      version: "1.2.3"
)==="), context);
	std::string compressed = strp.resolve_partial(vars).resolve(make_vars_manager(hs));
	assert_equals(R"===({"description":["Server ",{"text":"example.com","color":"gold","bold":true}," is down right now"],"favicon":"data:image/png;base64,dGV4dDogbm90IGltYWdl","modinfo":{"type":"FML","modList":[{"modid":"ComputerCraft","version":"1.2.3"}]},"version":{"name":")===" + config::project + "-" + config::version + R"===(","protocol":-1},"players":{"sample":[],"max":20,"online":0}})===",
		compressed);
	}

	{
	auto strp = build_status(YAML::Load(R"===(
refs: !ignore
  ktlo: &ktlo
    name: Ktlo
    id: popka

random: !ignore ignore

description: !ignore ignore
version: !ignore ignore
players:
  sample:
    - *ktlo
    - name: Rou7e
      id: zopka
)==="), context);
	std::string compressed = strp.resolve(vars);
	assert_equals(R"===({"players":{"sample":[{"name":"Ktlo","id":"popka"},{"name":"Rou7e","id":"zopka"}],"max":20,"online":2}})===",
		compressed);
	}

	{
	auto strp = build_status(YAML::Load(R"===(
description: !ignore ignore
players: !ignore ignore
version:
  name: CraftBaget
  protocol: 1337
)==="), context);
	std::string compressed = strp.resolve(vars);
	assert_equals(R"===({"version":{"name":"CraftBaget","protocol":1337}})===",
		compressed);
	}

	{
	auto strp = build_status(YAML::Load(R"===(
description: {"selector":"Selector","text":"Text","bold":on,"italic":on,"strikethrough":on,"underlined":on,"obfuscated":off,"color":"dark_red"}
players: !ignore ignore
version: !ignore ignore
)==="), context);
	std::string compressed = strp.resolve(vars);
	assert_equals(R"===({"description":{"selector":"Selector","text":"Text","bold":true,"italic":true,"strikethrough":true,"underlined":true,"obfuscated":false,"color":"dark_red"}})===",
		compressed);
	}

	{
	auto strp = build_status(YAML::Load(R"===(
description: !ignore ignore
players: !ignore ignore
version: !ignore ignore
some: !uknown popka
)==="), context);
	std::string compressed = strp.resolve(vars);
	assert_equals(R"===({"some":"popka"})===",
		compressed);
	}

	{
	auto strp = build_status(YAML::Load(R"===(
description: !ignore ignore
players: !ignore ignore
version: !ignore ignore
content: !file icon.txt
include: $file:icon.txt
)==="), context);
	std::string compressed = strp.resolve(vars);
	assert_equals(R"===({"content":{"text":"not image"},"include":"text: not image"})===",
		compressed);
	}

	{
	auto strp = build_status(YAML::Load(R"===({
    "version": {
        "name": "1.8.7",
        "protocol": 47
    },
    "players": {
        "max": 100,
        "online": 5,
        "sample": [
            {
                "name": "thinkofdeath",
                "id": "4566e69f-c907-48ee-8d71-d7ba5aa00d20"
            }
        ]
    },	
    "description": {
        "text": "Hello world"
    },
    "favicon": "data:image/png;base64,<data>"
})==="), context);
	std::string compressed = strp.resolve(vars);
	assert_equals(R"===({"version":{"name":"1.8.7","protocol":47},"players":{"max":100,"online":5,"sample":[{"name":"thinkofdeath","id":"4566e69f-c907-48ee-8d71-d7ba5aa00d20"}]},"description":{"text":"Hello world"},"favicon":"data:image/png;base64,<data>"})===",
		compressed);
	}

	assert_fails_with(response_type_error, {
	build_status(YAML::Load(R"===(
version:
  protocol: -3lol45
)==="), context);
	});

	srand(65443);
	for (int i = 0; i < 10; i++) {
		std::string id = ekutils::uuid::random();
		log_debug("uuid example: " + id);
	}
}
