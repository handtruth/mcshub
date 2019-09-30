#include "response_props.hpp"

#include <cstdlib>
#include <fstream>

#include <ekutils/uuid.hpp>

namespace mcshub {

const std::string nothing = "{ NULL }";
const std::string base_srv = ".";

file_vars::file_vars() : srv_name(base_srv) {}

std::string file_vars::operator[](const std::string & name) const {
	if (name.empty())
		return nothing;
	std::string path = name[0] == '/' ? name.substr(1) : srv_name.get() + '/' + name;
	std::ifstream file(path);
	if (file)
		return std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
	else
		return nothing;
}

img_vars::img_vars() : srv_name(base_srv) {}

char code2char(unsigned char c) {
	if (c <= 25)
		return c + 'A';
	else if (c <= 51)
		return c - 26 + 'a';
	else if (c <= 61)
		return c - 52 + '0';
	else if (c == 62)
		return '+';
	else
		return '/';
}

std::string img_vars::operator[](const std::string & name) const {
	if (name.empty())
		return nothing;
	std::string path = name[0] == '/' ? name.substr(1) : srv_name.get() + '/' + name;
	std::ifstream file(path);
	if (!file)
		return nothing;
	std::string result = "data:image/png;base64,";
	file.seekg(0, std::ios::end);
	std::size_t size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::size_t b64size = size * 4;
	b64size = b64size / 3 + (b64size % 3 ? 4 : 0);
	result.reserve(result.size() + b64size);
	for (std::size_t count = size / 3; count; count--) {
		std::array<char, 3> triplet;
		file.read(triplet.data(), triplet.size());
		std::array<char, 4> encoded;
		encoded[0] = code2char(unsigned(triplet[0]) >> 2u);
		encoded[1] = code2char(((triplet[0] & 0b11) << 4) | (unsigned(triplet[1]) >> 4u));
		encoded[2] = code2char(((triplet[1] & 0b1111) << 2) | (unsigned(triplet[2]) >> 6));
		encoded[3] = code2char(triplet[2] & 0b111111);
		result.append(encoded.data(), encoded.size());
	}
	std::size_t read = size % 3;
	if (read == 2) {
		std::array<char, 2> triplet;
		file.read(triplet.data(), triplet.size());
		std::array<char, 4> encoded;
		encoded[0] = code2char(unsigned(triplet[0]) >> 2u);
		encoded[1] = code2char(((triplet[0] & 0b11) << 4) | (unsigned(triplet[1]) >> 4u));
		encoded[2] = code2char((triplet[1] & 0b1111) << 2);
		encoded[3] = '=';
		result.append(encoded.data(), encoded.size());
	} else if (read == 1) {
		std::array<char, 1> triplet;
		file.read(triplet.data(), triplet.size());
		std::array<char, 4> encoded;
		encoded[0] = code2char(unsigned(triplet[0]) >> 2u);
		encoded[1] = code2char((triplet[0] & 0b11) << 4);
		encoded[2] = '=';
		encoded[3] = '=';
		result.append(encoded.data(), encoded.size());
	}
	return result;
}

std::string env_vars_t::operator[](const std::string & name) const {
	const char * result = std::getenv(name.c_str());
	if (result)
		return result;
	else
		return nothing;
}

env_vars_t env_vars;

std::string server_vars::operator[](const std::string & name) const {
	auto iter = vars->find(name);
	if (iter == vars->end())
		return nothing;
	else
		return iter->second;
}

std::string main_vars_t::operator[](const std::string & name) const {
	if (name == "uuid") {
		return ekutils::uuid::random();
	} else
		return nothing;
}

main_vars_t main_vars;

}
