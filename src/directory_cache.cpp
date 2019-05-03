#include "directory_cache.h"

namespace mcshub {

std::vector<std::string> directory_cache::find_new() {
	std::vector<std::string> result;
	auto end = names.end();
	for (const auto & file : fs::directory_iterator(dir_path)) {
		std::string name = file.path().filename();
		if (names.find(name) == end) {
			result.push_back(name);
			names.emplace(name);
		}
	}
	return result;
}

std::set<std::string> directory_cache::find_deleted() {
	std::set<std::string> reminds = names;
	for (const auto & file : fs::directory_iterator(dir_path)) {
		std::string name = file.path().filename();
		reminds.erase(name);
	}
	for (const std::string & name : reminds) {
		names.erase(name);
	}
	return reminds;
}

}
