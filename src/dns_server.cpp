#include "dns_server.h"

#include <cstring>
#include <stdexcept>
#include <sstream>

namespace mcshub {

inline std::uint16_t swp16int(std::uint16_t x) {
	return x >> 8 | x << 8;
}

inline std::uint32_t swp32int(std::uint32_t x) {
	return (swp16int(x >> 8) << 8) | x >> 24 | x << 24;
}

int dns_packet::header_t::read(const byte_t bytes[], size_t length) {
	if (length < size())
		return -1;
	std::memcpy(this, bytes, size());
	q_count = swp16int(q_count);
	a_count = swp16int(a_count);
	ns_count = swp16int(ns_count);
	rcode = swp16int(rcode);
	return size();
}

int dns_packet::header_t::write(byte_t bytes[], size_t length) const {
	if (length < size())
		return -1;
	std::memcpy(bytes, this, size());
	header_t * H = reinterpret_cast<header_t *>(bytes);
	H->q_count = swp16int(q_count);
	H->a_count = swp16int(a_count);
	H->ns_count = swp16int(ns_count);
	H->rcode = swp16int(rcode);
	return size();
}

size_t dns_packet::question_t::size() const noexcept {
	std::size_t len = name.size();
	return (name[len - 1] == '.' ? len : len + 1) + (sizeof(record_t) + sizeof(class_t));
}

int dns_packet::question_t::read(const byte_t bytes[], size_t length) {
	if (length < size())
		return -1;
	int i = 0;
	const signed len = static_cast<signed>(length);
	char gather[257];
	while (true) {
		byte_t byte = bytes[i];
		if (i + byte >= len)
			return -1;
		if (byte == 0)
			break;
		std::memcpy(gather + i, bytes + i + 1, byte);
		i += byte;
		gather[i] = '.';
		i++;
	}
	if (i <= 0)
		return -1;
	name = std::string(gather, i - 1);
	i++;
	type = (record_t)swp16int(*reinterpret_cast<const std::uint16_t *>(bytes + i));
	qclass = (class_t)swp16int(*reinterpret_cast<const std::uint16_t *>(bytes + i + sizeof(record_t)));
	return i + (sizeof(record_t) + sizeof(class_t));
}

int dns_packet::question_t::write(byte_t bytes[], size_t length) const {
	if (length < size())
		return -1;
	int pos = 0;
	int i = 0, j = 0;
	while (true) {
		j = name.find_first_of('.', i);
		if (j > 0) {
			byte_t len = j - i;
			bytes[pos] = len;
			std::memcpy(bytes + pos + 1, name.c_str() + i, len);
			pos += 1 + len;
		} else
			break;
		i = j + 1;
	}
	j = name.size();
	if (name[j - 1] != '.') {
		byte_t len = j - i;
		bytes[pos] = len;
		std::memcpy(bytes + pos + 1, name.c_str() + i, len);
		pos += 1 + len;
	}
	bytes[pos] = 0;
	pos++;
	*reinterpret_cast<record_t *>(bytes + pos) = record_t(swp16int(std::uint16_t(type)));
	*reinterpret_cast<class_t *>(bytes + pos + sizeof(record_t)) = class_t(swp16int(std::uint16_t(qclass)));
	return pos + (sizeof(record_t) + sizeof(class_t));
}

dns_packet::record_t dns_packet::answer_t::a_rdata::type() const noexcept {
	return record_t::A;
}

size_t dns_packet::answer_t::a_rdata::size() const noexcept {
	return 4;
}

int dns_packet::answer_t::a_rdata::read(const byte_t bytes[], size_t length) {
	if (length != 4)
		return -1;
	data = *reinterpret_cast<const std::uint32_t *>(bytes);
	return 4;
}

int dns_packet::answer_t::a_rdata::write(byte_t bytes[], size_t length) const {
	if (length < 4)
		return -1;
	*reinterpret_cast<std::uint32_t *>(bytes) = data;
	return 4;
}

dns_packet::record_t dns_packet::answer_t::aaaa_rdata::type() const noexcept {
	return record_t::AAAA;
}

size_t dns_packet::answer_t::aaaa_rdata::size() const noexcept {
	return 16;
}

int dns_packet::answer_t::aaaa_rdata::read(const byte_t bytes[], size_t length) {
	if (length != 16)
		return -1;
	std::memcpy(data, bytes, 16);
	return 16;
}

int dns_packet::answer_t::aaaa_rdata::write(byte_t bytes[], size_t length) const {
	if (length < 16)
		return -1;
	std::memcpy(bytes, data, 16);
	return 16;
}

size_t dns_packet::answer_t::ns_txt_rdata::size() const noexcept {
	return data.size();
}

int dns_packet::answer_t::ns_txt_rdata::read(const byte_t bytes[], size_t length) {
	data = std::string(reinterpret_cast<const char *>(bytes), length);
	return 16;
}

int dns_packet::answer_t::ns_txt_rdata::write(byte_t bytes[], size_t length) const {
	if (length < data.size())
		return -1;
	std::memcpy(bytes, data.c_str(), data.size());
	return 16;
}

dns_packet::record_t dns_packet::answer_t::ns_rdata::type() const noexcept {
	return record_t::NS;
}

dns_packet::record_t dns_packet::answer_t::txt_rdata::type() const noexcept {
	return record_t::TXT;
}

size_t dns_packet::answer_t::size() const noexcept {
	return sizeof(std::uint32_t) + sizeof(std::uint16_t) + rdata->size();
}

dns_packet::answer_t::rdata_t * create_rdata(dns_packet::record_t type) {
	switch (type) {
	case dns_packet::record_t::A:
		return new dns_packet::answer_t::a_rdata();
	case dns_packet::record_t::AAAA:
		return new dns_packet::answer_t::aaaa_rdata();
	case dns_packet::record_t::NS:
		return new dns_packet::answer_t::ns_rdata();
	case dns_packet::record_t::TXT:
		return new dns_packet::answer_t::txt_rdata();
	default:
		throw std::runtime_error("unsupported dns record type");
	}
}

int dns_packet::answer_t::read(const byte_t bytes[], size_t length) {
	int s = qread(bytes, length);
	if (s < 0)
		return -1;
	if (length < s + (sizeof(std::uint32_t) + sizeof(std::uint16_t)))
		return -1;
	ttl = swp32int(*reinterpret_cast<const std::uint32_t *>(bytes + s));
	size_t rsize = swp16int(*reinterpret_cast<const std::uint16_t *>(bytes + s + sizeof(std::uint32_t)));
	size_t len = s + (sizeof(std::uint32_t) + sizeof(std::uint16_t)) + rsize;
	if (length < len)
		return -1;
	if (rdata)
		delete rdata;
	rdata_t * new_rdata = create_rdata(type);
	new_rdata->read(bytes + s + (sizeof(std::uint32_t) + sizeof(std::uint16_t)), rsize);
	rdata = new_rdata;
	return len;
}

int dns_packet::answer_t::write(byte_t bytes[], size_t length) const {
	int s = qwrite(bytes, length);
	if (s < 0)
		return -1;
	if (length < s + (sizeof(std::uint32_t) + sizeof(std::uint16_t)))
		return -1;
	*reinterpret_cast<std::uint32_t *>(bytes + s) = swp32int(ttl);
	size_t rsize = rdata->size();
	*reinterpret_cast<std::uint16_t *>(bytes + s + sizeof(std::uint32_t)) = swp16int(rsize);
	size_t len = s + (sizeof(std::uint32_t) + sizeof(std::uint16_t)) + rsize;
	if (length < len)
		return -1;
	rdata->write(bytes + s + (sizeof(std::uint32_t) + sizeof(std::uint16_t)), rsize);
	return len;
}

size_t dns_packet::size() const {
	size_t sz = header.size();
	for (const question_t & question : questions)
		sz += question.size();
	for (const answer_t & answer : answers)
		sz += answer.size();
	return sz;
}

int dns_packet::read(const byte_t bytes[], size_t length) {
	int s = header.read(bytes, length);
	if (s < 0)
		return -1;
	questions.clear();// = std::vector<question_t>(header.q_count);
	for (int i = 0; i < header.q_count; i++) {
		questions.push_back(question_t());
		question_t & question = questions.back();
		int r = question.read(bytes + s, length - s);
		if (r < 0)
			return -1;
		s += r;
	}
	answers.clear();// = std::vector<answer_t>(header.a_count);
	for (int i = 0; i < header.a_count; i++) {
		questions.push_back(question_t());
		question_t & question = questions.back();
		int r = question.read(bytes + s, length - s);
		if (r < 0)
			return -1;
		s += r;
	}
	return s;
}

int dns_packet::write(byte_t bytes[], size_t length) const {
	header.q_count = questions.size();
	header.a_count = answers.size();
	int s = header.write(bytes, length);
	if (s < 0)
		return -1;
	for (const question_t & question : questions) {
		int r = question.write(bytes + s, length - s);
		if (r < 0)
			return -1;
		s += r;
	}
	for (const answer_t & answer : answers) {
		int r = answer.write(bytes + s, length - s);
		if (r < 0)
			return -1;
		s += r;
	}
	return s;
}

void dns_packet::answer_A(const std::string & name, std::uint32_t ip_addr, std::uint32_t ttl) {
	answer_t answer = answer_t();
	answer.name = name;
	answer.ttl = ttl;
	answer.qclass = class_t::IN;
	answer.type = record_t::A;
	auto * rdata = new answer_t::a_rdata(swp32int(ip_addr));
	answer.rdata = rdata;
	answers.push_back(std::move(answer));
}

std::shared_ptr<dns_packet::answer_t::rdata_t> dns_packet::parse_dns_record(const std::string & data) {
	std::stringstream scanner(data);
	std::string type_s;
	scanner >> type_s;
	std::string content;
	std::getline(scanner, content);
	std::shared_ptr<dns_packet::answer_t::rdata_t> result;
	if (type_s == "A") {
		std::stringstream ip_scanner(content);
		std::uint32_t ip = 0;
		std::string byte;
		int count = 0;
		while (std::getline(ip_scanner, byte, '.')) {
			count++;
			ip = (ip << 8) | std::stoi(byte);
		}
		if (count != 4)
			throw std::runtime_error("dns record parse problem \"" + data + "\": bad number of bytes in ip address");
		result = std::make_shared<dns_packet::answer_t::a_rdata>(ip);
	} else if (type_s == "AAAA") {
		throw std::runtime_error("NOT IMLEMENTED YET");
	} else if (type_s == "NS") {
		throw std::runtime_error("NOT IMLEMENTED YET");
	} else if (type_s == "TXT") {
		throw std::runtime_error("NOT IMLEMENTED YET");
	} else {
		throw std::runtime_error("unrecognised dns record type \"" + type_s + "\"");
	}
	return result;
}

} // mcshub
