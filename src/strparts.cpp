#include "strparts.hpp"

#include <cstring>
#include <limits>
#include <algorithm>

#include <ekutils/parse_essentials.hpp>
#include <ekutils/log.hpp>
#include <yaml-cpp/yaml.h>

namespace mcshub {

inline bool isvar(char c) {
	return std::isalnum(c) || c == ':' || c == '_' || c == '.' || c == '-';
}

strparts::address_t parse_var(const char * str, std::size_t length) {
	using namespace std::string_literals;
	ekutils::trim_string(str, length);
	int d = ekutils::find_char<':'>(str, length);
	if (d < 0)
		return strparts::address_t { ""s, std::string(str, length) };
	std::size_t ns_len = d;
	ekutils::trim_string(str, ns_len);
	std::string ns = std::string(str, ns_len);
	const char * name_raw = str + d + 1;
	std::size_t name_raw_len = length - d - 1;
	ekutils::trim_string(name_raw, name_raw_len);
	std::string name = std::string(name_raw, name_raw_len);
	return strparts::address_t { std::move(ns), std::move(name) };
}

strparts::strparts() {}

strparts::strparts(std::string && src) : source(std::move(src)) {
	unsigned i = static_cast<unsigned>(source.find('$'));
	if (i == std::numeric_limits<unsigned>::max())
		return; // string has no special cases, best situation
	unsigned j, length, buffer_size;
	const char * str = source.c_str();
	for (j = 0, buffer_size = 0, length = static_cast<unsigned>(source.length()); i < length;) {
		if (str[i] == '$') {
			if (!isvar(str[i + 1]) && str[i + 1] != '{' && str[i + 1] != '$') {
				i += 1;
				continue;
			}
			unsigned part_len = i - j;
			if (part_len)
				parts.emplace_back(segment_t { j, part_len });
			i++;
			switch (str[i]) {
				case '$': {
					parts.emplace_back(segment_t { i, 1 });
					i++;
					j = i;
					break;
				}
				case '{': {
					int t = ekutils::find_closing_bracket<'{'>(str + i);
					parts.emplace_back(parse_var(str + i + 1, t - 2));
					i += t;
					j = i;
					++buffer_size;
					break;
				}
				default: {
					std::size_t t;
					for (t = 0; isvar(str[i + t]); t++);
					parts.emplace_back(parse_var(str + i, t));
					i += t;
					j = i;
					++buffer_size;
					break;
				}
			}
		} else
			i++;
	}
	if (i != j)
		parts.emplace_back(segment_t { j, i - j });
	buffer.reserve(buffer_size);
}

bool strparts::simple() const noexcept {
	if (parts.empty())
		return true;
	for (const auto & part : parts) {
		if (std::holds_alternative<address_t>(part))
			return false;
	}
	return true;
}

void strparts::append(const strparts & other) {
	if (other.parts.empty()) {
		append(other.source);
	} else {
		for (const auto & each : other.parts) {
			if (std::holds_alternative<segment_t>(each)) {
				const auto & segment = std::get<segment_t>(each);
				append(segment.of(other.source));
			} else {
				const auto & address = std::get<address_t>(each);
				append(address.ns, address.var);
			}
		}
		buffer.reserve(buffer.capacity() + other.buffer.capacity());
	}
}

void strparts::append_jstr(const strparts & other) {
	if (other.parts.empty()) {
		YAML::Emitter emitter;
		emitter << YAML::Flow << YAML::DoubleQuoted << other.source;
		append(emitter.c_str());
	} else {
		append("\"");
		std::string strbuff;
		for (const auto & each : other.parts) {
			if (std::holds_alternative<segment_t>(each)) {
				const auto & segment = std::get<segment_t>(each);
				strbuff = segment.of(other.source);
				YAML::Emitter emitter;
				emitter << YAML::Flow << YAML::DoubleQuoted << strbuff;
				const char * outstr = emitter.c_str();
				std::size_t size = std::strlen(outstr);
				append(std::string_view(outstr + 1, size - 2));
			} else {
				const auto & address = std::get<address_t>(each);
				append(address.ns, address.var);
			}
		}
		append("\"");
		buffer.reserve(buffer.capacity() + other.buffer.capacity());
	}
}

void strparts::append(const std::string & string) {
	unsigned size = static_cast<unsigned>(source.length());
	source.append(string);
	parts.emplace_back(segment_t { size, static_cast<unsigned>(string.size()) });
}

void strparts::append(const std::string_view & string) {
	unsigned size = static_cast<unsigned>(source.length());
	source.append(string);
	parts.emplace_back(segment_t { size, static_cast<unsigned>(string.size()) });
}

void strparts::append(const char * string) {
	unsigned size = static_cast<unsigned>(source.length());
	unsigned length = static_cast<unsigned>(std::strlen(string));
	source.append(string, length);
	parts.emplace_back(segment_t { size, length });
}

void strparts::append(const std::string & ns, const std::string & var) {
	source.append("${");
	source.append(ns);
	source.append(":");
	source.append(var);
	source.append("}");
	parts.emplace_back(address_t { ns, var });
}

void strparts::simplify() {
	if (parts.size() < 2)
		return;
	auto ita = parts.begin();
	auto itb = ita + 1;
	while (itb != parts.end()) {
		auto & a = *ita;
		auto & b = *itb;
		if (std::holds_alternative<segment_t>(a) && std::holds_alternative<segment_t>(b)) {
			const auto & stra = std::get<segment_t>(a);
			const auto & strb = std::get<segment_t>(b);
			if (stra.offset + stra.size == strb.offset) {
				a = segment_t { stra.offset, stra.size + strb.size };
				parts.erase(itb);
				continue;
			}
		}
		++ita;
		++itb;
	}
	std::remove_if(parts.begin(), parts.end(), [](const part_t & part) -> bool {
		if (std::holds_alternative<segment_t>(part)) {
			const auto & segment = std::get<segment_t>(part);
			return segment.size == 0;
		}
		return false;
	});
	if (parts.size() == 1u && std::holds_alternative<segment_t>(parts[0])) {
		const segment_t & segment = std::get<segment_t>(parts[0]);
		source.erase(0, segment.offset);
		source.erase(segment.size);
		parts.clear();
	}
	parts.shrink_to_fit();
}

strparts strparts::resolve_partial(const vars_base & vars) const {
	strparts result;
	if (parts.empty()) {
		result.source = source;
		return result;
	}
	std::size_t buffer_cnt = 0;
	for (const part_t & part : parts) {
		if (std::holds_alternative<segment_t>(part)) {
			const segment_t & view = std::get<segment_t>(part);
			result.append(view.of(source));
		} else {
			const address_t & address = std::get<address_t>(part);
			std::optional<std::string> opt = vars.get(address.ns, address.var);
			if (opt.has_value()) {
				result.append(opt.value());
			} else {
				result.append(address.ns, address.var);
				++buffer_cnt;
			}
		}
	}
	result.buffer.reserve(buffer_cnt);
	result.simplify();
	result.source.shrink_to_fit();
	return result;
}

std::string strparts::resolve(const vars_base & vars) const {
	using namespace std::string_literals;
	if (parts.empty()) {
		return source;
	}
	buffer.clear();
	std::size_t size = 0;
	for (const part_t & part : parts) {
		if (std::holds_alternative<segment_t>(part)) {
			size += std::get<segment_t>(part).size;
		} else {
			const address_t & address = std::get<address_t>(part);
			std::optional<std::string> opt = vars.get(address.ns, address.var);
			const std::string & value = buffer.emplace_back(
				opt.value_or("${"s + address.ns + ':' + address.var + "}")
			);
			size += value.size();
		}
	}
	std::string result;
	result.reserve(size);
	std::size_t buffer_i = 0;
	for (const part_t & part : parts) {
		if (std::holds_alternative<segment_t>(part)) {
			result.append(std::get<segment_t>(part).of(source));
		} else {
			result.append(buffer[buffer_i++]);
		}
	}
	return result;
}

} // namespace mcshub
