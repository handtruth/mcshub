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
