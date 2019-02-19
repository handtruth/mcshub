#include "log.h"

#include <array>
#include <typeinfo>
#include <iostream>

namespace mcshub {

const std::array<const std::string, (int)log_level::debug + 1> lvl_strs {
	"none", "fatal", "error", "warning", "info", "verbose", "debug"
};

const std::string & log_lvl2str(log_level lvl) {
	return lvl_strs[(int)lvl];
}

void log_base::write_private(log_level level, const std::string & message) {
	if ((int)level > (int)lvl)
		return;
	write("[" + lvl_strs[(int)level] + "]:" + ' ' + message);
}

void log_base::write_exception(log_level level, const std::exception exception) {
	if ((int)level > (int)lvl)
		return;
	write("[" + lvl_strs[(int)level] + "]:" + ' ' + typeid(exception).name() + ": " + exception.what());
}

void file_log::write(const std::string & message) {
	output << message << std::endl;
}

void stdout_log::write(const std::string & message) {
	std::cout << message << std::endl;
}

empty_log no_log;

log_base * log = &no_log;

} // mcshub
