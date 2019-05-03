#include "response_props.h"

#include <cstdlib>
#include <fstream>

#include "uuid.h"

namespace mcshub {

std::string file_vars::operator[](const std::string & name) const {
	std::ifstream file(srv_name + '/' + name);
	if (file)
		return std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
	else
		return "{ NULL }";
}

std::string env_vars_t::operator[](const std::string & name) const {
	const char * result = std::getenv(name.c_str());
	if (result)
		return result;
	else
		return "{ NULL }";
}

env_vars_t env_vars;

std::string server_vars::operator[](const std::string & name) const {
	auto iter = vars->find(name);
	if (iter == vars->end())
		return "{ NULL }";
	else
		return iter->second;
}

std::string main_vars_t::operator[](const std::string & name) const {
	if (name == "uuid") {
		return uuid::random();
	} else
		return "{ NULL }";
}

main_vars_t main_vars;

}
