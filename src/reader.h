#ifndef _READER_HEAD
#define _READER_HEAD

#include <string>

#include "reallobuf.h"
#include "in_stream.h"

namespace mcshub {

class reader {
	reallobuf buff;
	size_t rem = 0;
	in_stream & input;
	void fetch();
	char getc();
	void gets(std::string & string, size_t length);
public:
	reader(in_stream & stream) : input(stream) {}
	char read();
	size_t read(std::string & string, size_t length);
	bool read_line(std::string & line);
};

} // namespace mcshub

#endif // _READER_HEAD
