#include "paket.h"

#include <exception>
#include <limits>
#include <cstring>
#include "log.h"

namespace mcshub {

size_t fields::varint::size() const noexcept {
	return size_varint(value);
}

int fields::varint::read(const byte_t bytes[], size_t length) {
	return read_varint(bytes, length, value);
}

int fields::varint::write(byte_t bytes[], size_t length) const {
	return write_varint(bytes, length, value);
}

fields::varint::operator std::string() const {
	return std::to_string(value);
}

size_t fields::string::size() const noexcept {
	size_t length = static_cast<size_t>(value.size());
	return size_varint(length) + length;
}

int fields::string::read(const byte_t bytes[], size_t length) {
	std::int32_t str_len;
	int s = read_varint(bytes, length, str_len);
	if (s < 0)
		return -1;
	if (str_len < 0)
		throw paket_error("string field is lower than 0");
	size_t reminder = length - s;
	size_t ustr_len = static_cast<size_t>(str_len);
	if (reminder < ustr_len)
		return -1;
	value.assign(reinterpret_cast<const char *>(bytes + s), ustr_len);
	return s + str_len;
}

int fields::string::write(byte_t bytes[], size_t length) const {
	std::int32_t str_len = static_cast<std::int32_t>(value.size());
	int s = write_varint(bytes, length, str_len);
	if (s < 0)
		return -1;
	size_t reminder = length - s;
	size_t ustr_len = static_cast<size_t>(str_len);
	if (reminder < ustr_len)
		return -1;
	std::memcpy(bytes + s, value.c_str(), ustr_len);
	return s + str_len;
}

fields::string::operator std::string() const {
	return '"' + value + '"';
}

size_t size_varint(std::int32_t value) {
	return static_cast<size_t>(write_varint(nullptr, std::numeric_limits<size_t>::max(), value));
}

int read_varint(const byte_t bytes[], size_t length, std::int32_t & value) {
	size_t numRead = 0;
    byte_t read;
	value = 0;
    do {
		if (numRead == length)
			return -1;
        read = bytes[numRead];
        int tmp = (read & 0b01111111);

		value |= (tmp << (7 * numRead));

        numRead++;
        if (numRead > 5) {
            throw std::range_error("VarInt is too big");
        }
    } while ((read & 0b10000000) != 0);
    return numRead;
}

int write_varint(byte_t bytes[], size_t length, std::int32_t value) {
	size_t numWrite = 0;
	std::uint32_t uval = value;
	do {
		if (numWrite == length)
			return -1;
		byte_t temp = (byte_t)(uval & 0b01111111);
		uval >>= 7;
		if (uval != 0) {
			temp |= 0b10000000;
		}
		if (bytes)
			bytes[numWrite++] = temp;
		else
			numWrite++;
	} while (uval != 0);
	return numWrite;
}

int pakets::head(const byte_t bytes[], size_t length, std::int32_t & size, std::int32_t & id) {
	int s = read_varint(bytes, length, size);
	if (s < 0)
		return -1;
	int k = read_varint(bytes + s, length - s, id);
	if (k < 0)
		return -1;
	else
		return s;
}

}
