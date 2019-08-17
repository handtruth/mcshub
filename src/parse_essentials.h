#ifndef _PARSE_ESSENTIALS_HEAD
#define _PARSE_ESSENTIALS_HEAD

namespace mcshub {

template <char oppening>
constexpr inline char get_closing_bracket() {
	static_assert(oppening == '{' || oppening == '(' || oppening == '<' || oppening == '[');
	return '\0';
}
template <>
constexpr inline char get_closing_bracket<'{'>() {
	return '}';
}
template <>
constexpr inline char get_closing_bracket<'('>() {
	return ')';
}
template <>
constexpr inline char get_closing_bracket<'<'>() {
	return '>';
}
template <>
constexpr inline char get_closing_bracket<'['>() {
	return ']';
}

template <char oppening>
int find_closing_bracket(const char * str) {
	constexpr char closing = get_closing_bracket<oppening>();
	int count = 0;
	int i = 0;
	int last = 0;
	do switch (str[i++]) {
		case oppening:
			count++;
			last = i - 1;
			break;
		case closing:
			count--;
			break;
		case '\0':
			return -last;
	} while (count);
	return i;
}

inline void skip_space(const char *& str) {
	while (std::isspace(*str)) {
		str++;
	}
}

inline size_t find_space(const char * str) {
	size_t length = 0;
	char c = *str;
	while (!std::isspace(c) && c != '\0') {
		c = str[++length];
	}
	return length;
}

inline size_t measure_word(const char * str) {
	size_t length = 0;
	char c = *str;
	while (std::isalnum(c) || c == '_' || c == '_' || c == '.') {
		c = str[++length];
	}
	return length;
}

inline size_t find_escaped_end(const char * str) {
	size_t length = 0;
	char c = *str;
	while (true) {
		if (c == '\0')
			throw std::runtime_error("end of string");
		if (c == '"')
			break;
		if (c == '\\' && str[length + 1] == '"')
			++length;
		c = str[++length];
	}
	return length + 1;
}

inline std::string unescape(const char * str, size_t length) {
	std::string result;
	result.reserve(length);
	for (size_t i = 0; i < length; i++) {
		if (str[i] == '\\') {
			switch (str[i + 1]) {
			case 'n':
				i++;
				result.push_back('\n');
				break;
			case '"':
				i++;
				result.push_back('"');
				break;
			case '\'':
				i++;
				result.push_back('\'');
				break;
			case '\\':
				i++;
				result.push_back('\\');
				break;
			case 't':
				i++;
				result.push_back('\t');
				break;
			case '0':
				i++;
				result.push_back('\0');
				break;
			case 'r':
				i++;
				result.push_back('\r');
				break;
			default:
				result.push_back('\\');
				break;
			}
		} else {
			result.push_back(str[i]);
		}
	}
	return result;
}

inline void trim_string(const char *& str, size_t & length) {
	if (length == 0) return;
	while (std::isspace(*str)) {
		str++;
		if (!(--length))
			return;
	}
	while (std::isspace(str[length - 1])) {
		if (!(--length))
			return;
	}
}

template <char c>
int find_char(const char * str, size_t length) {
	for (size_t i = 0; i < length; i++) {
		switch (str[i]) {
		case '\0':
			return -1;
		case c:
			return i;
		}
	}
	return -1;
}

}

#endif // _PARSE_ESSENTIALS_HEAD
