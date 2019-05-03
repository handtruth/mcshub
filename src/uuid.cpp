#include "uuid.h"

#include <cstdlib>

namespace mcshub {

inline std::uint64_t urand32() {
	return static_cast<std::uint64_t>(std::rand());
}
inline std::uint64_t urand64() {
	return (urand32() << 32) | urand32();
}

uuid uuid::random() noexcept {
	return uuid(urand64(), urand64());
}

inline char to_hex_digit(byte_t digit) {
	return (digit >= 0 && digit <= 9) ? (digit + '0') : (digit + ('a' - 10));
}

void insert_uuid_part(char result[], const byte_t bytes[], size_t count) {
	for (; count--; bytes++, result += 2) {
		char most = to_hex_digit((*bytes >> 4) & 15);
		char least = to_hex_digit(*bytes & 15);
		result[0] = most;
		result[1] = least;
	}
}

uuid::operator std::string() const {
	char result[36]{};
	//xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
	insert_uuid_part(result, bytes, 4);
	result[8] = '-';
	insert_uuid_part(result + 9, bytes + 4, 2);
	result[13] = '-';
	insert_uuid_part(result + 14, bytes + 6, 2);
	result[18] = '-';
	insert_uuid_part(result + 19, bytes + 8, 2);
	result[23] = '-';
	insert_uuid_part(result + 24, bytes + 10, 6);
	return std::string(result, 36);
}

}
