#ifndef STATUS_BUILDER_HEAD_SDDPAKVO
#define STATUS_BUILDER_HEAD_SDDPAKVO

#include <yaml-cpp/yaml.h>

#include "strparts.hpp"

namespace mcshub {

struct response_type_error : public std::runtime_error {
	response_type_error(const std::string & message) : std::runtime_error(message) {}
};

struct vars_prohobited_error : public std::runtime_error {
	vars_prohobited_error(const std::string & message) : std::runtime_error(message) {}
};

strparts build_chat(const YAML::Node & node, const std::string & context);
strparts build_status(const YAML::Node & node, const std::string & context);

} // namespace mcshub

#endif // STATUS_BUILDER_HEAD_SDDPAKVO
