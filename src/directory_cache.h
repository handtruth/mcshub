#ifndef _DIRECTORY_CACHE_HEAD
#define _DIRECTORY_CACHE_HEAD

#include <vector>
#include <set>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace mcshub {

class directory_cache final {
private:
	fs::path dir_path;
	std::set<std::string> names;
public:
	directory_cache(fs::path path) : dir_path(path) {}

	std::vector<std::string> find_new();
	std::set<std::string> find_deleted();
};

} // mcshub

#endif
