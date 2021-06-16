#include "gate.hpp"

#include <cassert>

#include "mc_pakets.hpp"
#include "settings.hpp"

namespace mcshub {

bool gate::head(std::int32_t & id, std::int32_t & size) const {
	int s = pakets::head(input.data(), input.size(), size, id);
	if (s == -1)
		return false;
	if (size < 0)
		throw bad_request("packet size is lower than 0");
	conf_snap conf;
	std::int32_t max_size = conf->max_packet_size;
	if (max_size != -1 && size > max_size)
		throw bad_request("the maximum allowed packet size was reached");
	size += s;
	return std::size_t(size) <= input.size();
}

void gate::kostilA() {
	pakets::request req;
	std::size_t sz = req.size() + 10;
	input.asize(sz);
	int r = req.write(input.data() + input.size() - sz, sz);
	assert(r != -1);
	input.ssize(sz - r);
}

void gate::kostilB(const std::string & nick) {
	pakets::login login;
	login.name() = nick;
	std::size_t size = login.size() + 10;
	input.asize(size);
	int r = login.write(input.data() + input.size() - size, size);
	assert(r != -1);
	input.ssize(size - r);
}

void gate::tunnel(gate & other) {
	other.output.append(input.data(), input.size());
	input.clear();
	other.send();
}

void gate::receive() {
	std::size_t avail = sock->avail();
	std::size_t old = input.size();
	input.asize(avail);
	sock->read(input.data() + old, avail);
}

void gate::send() {
	if (output.size() == 0)
		return;
	int written = sock->write(output.data(), output.size());
	if (written == -1)
		return;
	output.move(written);
}

} // namespace mcshub
