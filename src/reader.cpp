#include "reader.h"

#include <cstring>

namespace mcshub {

const size_t part = 256;

void reader::fetch() {
	int s = 0;
	do {
	buff.probe(rem + part);
	s = input.read(buff.data() + rem, part);
	if (s == -1)
		return;
	rem += s;
	} while (s == part);
}

char reader::getc() {
	if (rem) {
		char c = *buff.data();
		std::memmove(buff.data(), buff.data() + 1, --rem);
		return c;
	}
	return -1;
}

void reader::gets(std::string & string, size_t length) {
	string.append(reinterpret_cast<const char *>(buff.data()), length);
	std::memmove(buff.data(), buff.data() + length, rem -= length);
}

char reader::read() {
	char r = getc();
	if (r != -1)
		return r;
	fetch();
	r = getc();
	if (r != -1)
		return r;
	return -1;
}

size_t reader::read(std::string & string, size_t length) {
	if (rem < length)
		fetch();
	size_t toread = std::min(rem, length);
	gets(string, toread);
	return toread;
}

bool reader::read_line(std::string & line) {
	size_t i;
	auto ptr = buff.data();
	for (i = 0; i < rem; i++) {
		if (ptr[i] == '\n') {
			gets(line, i);
			getc();
			return true;
		}
	}
	fetch();
	for (; i < rem; i++) {
		if (ptr[i] == '\n') {
			gets(line, i);
			getc();
			return true;
		}
	}
	return false;
}

} // namespace mcshub
