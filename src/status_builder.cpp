#include "status_builder.hpp"

#include <sstream>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <unordered_set>

#include <ekutils/log.hpp>

#include "config.hpp"
#include "response_props.hpp"

namespace fs = std::filesystem;

namespace mcshub {

namespace yamltags {
#	define YAML_TAG "tag:yaml.org,2002:"
// LCOV_EXCL_START
	const std::string & yint = YAML_TAG "int";
	const std::string & yfloat = YAML_TAG "float";
	const std::string & ystr = YAML_TAG "str";
	const std::string & ybool = YAML_TAG "bool";
	const std::string & ynull = YAML_TAG "null";
	const std::string & yseq = YAML_TAG "seq";
	const std::string & ymap = YAML_TAG "map";
	const std::string & file = "!file";
	const std::string & img = "!img";
	const std::string & ignore = "!ignore";
	const std::string & none = "?";
// LCOV_EXCL_STOP
#	undef YAML_TAG
}

void putJSONString(strparts & stream, const std::string & string) {
	YAML::Emitter emitter;
	emitter << YAML::Flow << YAML::DoubleQuoted << string;
	stream.append(emitter.c_str());
}

class json_stream {
	const std::string & context;
	strparts stream;
	bool not_first = false;
	void next() {
		if (not_first)
			putc(',');
		else
			not_first = true;
	}
	bool is_var(const std::string & value) {
		return value[0] == '$' && value[1] == '{' && value[value.length() - 1] == '}';
	}
	bool is_int(const std::string & value) {
		for (std::size_t i = value[0] == '-', length = value.length(); i < length; i++) {
			if (value[i] > '9' || value[i] < '0')
				return false;
		}
		return true;
	}
	bool is_float(const std::string & value) {
		YAML::Node node = YAML::Load(value);
		try {
			node.as<double>();
			return true;
		} catch (...) {
			return false;
		}
	}
	void assert_type(bool value, const std::string & type) {
		if (!value)
			throw response_type_error("this yaml literal can not be converted to " + type);
	}
public:
	strparts commit() {
		stream.simplify();
		return stream;
	}
	json_stream(const std::string & ctxt) : context(ctxt) {}
	void putc(char c) {
		stream.append(std::string_view(&c, 1));
	}
	typedef std::function<const std::string &(const fs::path &)> type_resolver;
	void put_node_internal(const YAML::Node & node, const fs::path & path, const type_resolver & resolver);
	void put_node(const std::string & name, const YAML::Node & node, const fs::path & path, const type_resolver & resolver) {
		next();
		putJSONString(stream, name);
		putc(':');
		put_node_internal(node, path/name, resolver);
	}
	void put_node(const YAML::Node & node, const fs::path & path, const type_resolver & resolver) {
		next();
		put_node_internal(node, path/"@", resolver);
	}
	void put_node(const YAML::Node & node, const type_resolver & resolver) {
		next();
		put_node_internal(node, fs::path("/"), resolver);
	}
	template <char a, char b>
	friend class json_block;
};

template <char open, char close>
struct json_block {
	json_stream & output;
	json_block(json_stream & stream) : output(stream) {
		stream.putc(open);
		stream.not_first = false;
	}
	~json_block() {
		output.putc(close);
		output.not_first = true;
	}
};

using json_struct = json_block<'{', '}'>;
using json_array = json_block<'[', ']'>;

void assert_simple_parts(const strparts & parts, const YAML::Node & node) {
	if (!parts.simple()) {
		const auto & mark = node.Mark();
		throw vars_prohobited_error("variables in node of type " + node.Tag() + " not supported (at line " +
			std::to_string(mark.line) + ", column " + std::to_string(mark.column) + ")");
	}
}

void json_stream::put_node_internal(const YAML::Node & node, const fs::path & path, const json_stream::type_resolver & resolver) {
	const std::string & type = node.Tag() == yamltags::none ? resolver(path) : node.Tag();
	switch (node.Type()) {
		case YAML::NodeType::Scalar: {
			strparts parts(node.as<std::string>());
			if (type == yamltags::img) {
				img_vars img;
				img.srv_name = context;
				assert_simple_parts(parts, node);
				putJSONString(stream, img[parts.resolve()]);
			} else if (type == yamltags::ystr) {
				stream.append_jstr(parts);
			} else if (type == yamltags::ynull) {
				stream.append("null");
			} else if (type == yamltags::ybool) {
				if (!parts.simple()) {
					stream.append(parts);
				} else {
					YAML::Node bool_node = YAML::Load(parts.resolve());
					if (bool_node.as<bool>())
						stream.append("true");
					else
						stream.append("false");
				}
			} else if (type == yamltags::yint) {
				if (parts.simple()) {
					auto value = parts.resolve();
					assert_type(is_int(value), yamltags::yint);
					stream.append(value);
				} else {
					stream.append(parts);
				}
			} else if (type == yamltags::yfloat) {
				if (parts.simple()) {
					auto value = parts.resolve();
					assert_type(is_float(value), yamltags::yfloat);
					stream.append(value);
				} else {
					stream.append(parts);
				}
			} else {
				if (node.Tag() != yamltags::none && node.Tag() != "!")
					log_warning("unknown type: " + node.Tag());
				stream.append_jstr(parts);
			}
			break;
		}
		case YAML::NodeType::Undefined:
		case YAML::NodeType::Null:
			stream.append("null");
			break;
		case YAML::NodeType::Sequence: {
			json_array js(*this);
			for (const auto & sub : node) {
				if (sub.Tag() != yamltags::ignore)
					put_node(sub, path, resolver);
			}
			break;
		}
		case YAML::NodeType::Map: {
			json_struct js(*this);
			for (const auto & sub : node) {
				if (sub.second.Tag() != yamltags::ignore)
					put_node(sub.first.as<std::string>(), sub.second, path, resolver);
			}
			break;
		}
	}
}

struct tree_load_ctxt {
	const std::string & context;
	std::unordered_set<std::string> files;
};

bool load_all_tree(YAML::Node & node, tree_load_ctxt & ctxt) {
	bool updated = false;
	if (node.Tag() == yamltags::file) {
		std::string file = node.as<std::string>();
		auto path = fs::canonical(file[0] == '/' ? "." + file : ctxt.context + '/' + file).string();
		auto it = ctxt.files.emplace(path);
		if (!it.second)
			throw std::runtime_error("file \"" + file + "\" already loaded in this tree, loops are not supported");
		node.reset(YAML::LoadFile(path));
		updated = true;
	}
	switch (node.Type()) {
	case YAML::NodeType::Map: {
		for (auto child : node) {
			auto & value = child.second;
			if (load_all_tree(value, ctxt))
				node[child.first] = value;
		}
		break;
	}
	case YAML::NodeType::Sequence: {
		std::size_t i = 0;
		for (YAML::Node child : node) {
			if (load_all_tree(child, ctxt))
				node[i] = child;
			++i;
		}
		break;
	}
	default:
		break;
	}
	return updated;
}

YAML::Node load_all_tree(const YAML::Node & input, const std::string & context) {
	YAML::Node node = input;
	tree_load_ctxt ctxt { context, { } };
	load_all_tree(node, ctxt);
	return node;
}

const std::string & chat_resolver(const fs::path & path) {
	if (!path.has_filename())
		return yamltags::ymap;
	const auto & name = path.filename();
	if (name == "text" || name == "selector" || name == "color")
		return yamltags::ystr;
	else if (name == "bold" || name == "italic" || name == "underlined"
			|| name == "strikethrough" || name == "obfuscated")
		return yamltags::ybool;
	else if (name == "extra")
		return yamltags::yseq;
	else if (name == "@")
		return yamltags::ymap;
	else
		return yamltags::none;
}

void build_chat(const YAML::Node & node, json_stream & stream) {
	stream.put_node(node, &chat_resolver);
}

strparts build_chat(const YAML::Node & node, const std::string & context) {
	json_stream stream(context);
	build_chat(load_all_tree(node, context), stream);
	return stream.commit();
}

const std::string & status_path_resolver(const fs::path & path) {
	if (path == "/")
		return yamltags::ymap;
	else if (path == "/version")
		return yamltags::ymap;
	else if (path == "/version/name")
		return yamltags::ystr;
	else if (path == "/version/protocol")
		return yamltags::yint;
	else if (path == "/players/max")
		return yamltags::yint;
	else if (path == "/players/online")
		return yamltags::yint;
	else if (path == "/players/sample")
		return yamltags::yseq;
	else if (path == "/players/sample/@")
		return yamltags::ymap;
	else if (path == "/players/sample/@/name")
		return yamltags::ystr;
	else if (path == "/players/sample/@/id")
		return yamltags::ystr;
	else if (path == "/favicon")
		return yamltags::ystr;
	else if (*(++path.begin()) == "description")
		return chat_resolver(path);
	else
		return yamltags::none;
}

const char * yaml_type2str(YAML::NodeType::value type) {
	switch (type) {
		case YAML::NodeType::Null:
			return "null";
		case YAML::NodeType::Scalar:
			return "scalar";
		case YAML::NodeType::Undefined:
			return "nothing";
		case YAML::NodeType::Map:
			return "map";
		case YAML::NodeType::Sequence:
			return "sequence";
		default:
			return "?";
	}
}

void assert_node_type(const YAML::Node & node, YAML::NodeType::value type) {
	using namespace std::string_literals;
	YAML::NodeType::value this_type = node.Type();
	if (this_type != type && this_type != YAML::NodeType::Undefined) {
		const auto & mark = node.Mark();
		throw response_type_error("invalid type in response object at [line "s
			+ std::to_string(mark.line) + ", column " + std::to_string(mark.column) + "] ("
			+ yaml_type2str(type) + " expected, got " + yaml_type2str(this_type) + ")");
	}
}

strparts build_status(const YAML::Node & node, const std::string & context) {
	YAML::Node tnode = load_all_tree(node, context);
	{
		auto version = tnode["version"];
		if (version.Tag() != yamltags::ignore) {
			auto name = version["name"];
			if (!name) {
				name = config::project + "-" + config::version;
				name.SetTag(yamltags::ystr);
			}
			auto protocol = version["protocol"];
			if (!protocol) {
				protocol = -1;
				protocol.SetTag(yamltags::yint);
			}
		}
	}
	{
		auto players = tnode["players"];
		if (players.Tag() != yamltags::ignore) {
			if (!players)
				players.SetTag(yamltags::ymap);
			auto sample = players["sample"];
			std::size_t online_count;
			if (sample) {
				online_count = sample.size();
				for (std::size_t i = 0; i < online_count; i++) {
					auto each = sample[i];
					switch (each.Type()){
					case YAML::NodeType::Map: {
						auto id = each["id"];
						if (!id) {
							id = main_vars["uuid"];
							id.SetTag(yamltags::ystr);
						}
						break;
					}
					case YAML::NodeType::Scalar: {
						YAML::Node player;
						player.SetTag(yamltags::ymap);
						auto name = player["name"];
						name = each.as<std::string>();
						name.SetTag(yamltags::ystr);
						auto id = player["id"];
						id = main_vars["uuid"];
						id.SetTag(yamltags::ystr);
						sample[i] = player;
					}
					default:
						break;
					}
				}
			} else {
				players["sample"] = YAML::Node(YAML::NodeType::Sequence);
				online_count = 0;
			}
			auto max = players["max"];
			if (!max) {
				max = std::max(std::size_t(20), online_count);
				max.SetTag(yamltags::yint);
			}
			auto online = players["online"];
			if (!online) {
				online = online_count;
				online.SetTag(yamltags::yint);
			}
		}
	}
	{
		auto description = tnode["description"];
		if (description.Tag() != yamltags::ignore && !description) {
			description = "Minecraft Server";
			description.SetTag(yamltags::ystr);
		}
	}
	json_stream stream(context);
	stream.put_node(tnode, &status_path_resolver);
	return stream.commit();
}

} // namespace mcshub
