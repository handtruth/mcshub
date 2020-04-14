#include <iostream>

#include <yaml-cpp/yaml.h>

#include "test.hpp"

test {
    using namespace std::string_literals;
    YAML::Emitter out;
    out << YAML::BeginMap << YAML::Key << "vars" << YAML::Anchor("vars") << YAML::VerbatimTag("ignore")
        << YAML::Value << YAML::BeginMap
        << YAML::Key << "name" << YAML::Anchor("vars.name") << YAML::SecondaryTag("str") << YAML::Value << "Server Name"
        << YAML::EndMap << YAML::EndMap;
    std::cout << out.c_str() << std::endl;
    YAML::Node node = YAML::Load(out.c_str() + R"==(
hello: *vars.name
)=="s);
    node.remove("vars");
    std::cout << node << std::endl;

    YAML::Node popka = YAML::Load(R"==(
lolka: !file /hello.yml
)==");
    std::cout << popka << std::endl << popka["lolka"].Tag() << std::endl;
}
