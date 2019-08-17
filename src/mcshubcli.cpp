#include "mcshubcli.h"

#include <cstring>

#include "parse_essentials.h"

namespace mcshub {

cli_form::~cli_form() {

}

cli_node::~cli_node() {
	
}

std::shared_ptr<choice_cli_node> chain_cli_node::choice() {
	auto ptr = std::make_shared<choice_cli_node>();
	chain(ptr);
	return ptr;
}

std::shared_ptr<word_cli_node> chain_cli_node::word(const std::string & form) {
	auto ptr = std::make_shared<word_cli_node>(form);
	chain(ptr);
	return ptr;
}

std::shared_ptr<option_cli_node> chain_cli_node::option(
		const std::string & form,
		const std::unordered_set<std::string> & options) {
	auto ptr = std::make_shared<option_cli_node>(form, options);
	chain(ptr);
	return ptr;
}

std::shared_ptr<form_cli_node> chain_cli_node::form(
		const std::string & form,
		const std::function<cli_form_ptr(void)> & factory,
		const std::map<std::string, std::string> & fields) {
	auto ptr = std::make_shared<form_cli_node>(form, factory, fields);
	chain(ptr);
	return ptr;
}

void choice_cli_node::add(const std::string & name, const cli_node_ptr & node) {
	children[name] = node;
}

std::shared_ptr<choice_cli_node> choice_cli_node::choice(const std::string & name) {
	auto ptr = std::make_shared<choice_cli_node>();
	add(name, ptr);
	return ptr;
}

std::shared_ptr<word_cli_node> choice_cli_node::word(const std::string & name, const std::string & form) {
	auto ptr = std::make_shared<word_cli_node>(form);
	add(name, ptr);
	return ptr;
}

std::shared_ptr<option_cli_node> choice_cli_node::option(
		const std::string & name,
		const std::string & form,
		const std::unordered_set<std::string> & options) {
	auto ptr = std::make_shared<option_cli_node>(form, options);
	add(name, ptr);
	return ptr;
}

std::shared_ptr<form_cli_node> choice_cli_node::form(
		const std::string & name,
		const std::string & form,
        const std::function<cli_form_ptr(void)> & factory,
        const std::map<std::string, std::string> & fields) {
	auto ptr = std::make_shared<form_cli_node>(form, factory, fields);
	add(name, ptr);
	return ptr;
}

void choice_cli_node::process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) {
	size_t word_sz = find_space(str);
	std::string next(str, word_sz);
	str += word_sz;
	skip_space(str);
	auto it = children.find(next);
	if (it != children.end())
		it->second->process(str, forms);
	else
		throw cli_choice_error("no command \"" + next + "\"");
}

void generate_help(std::vector<std::string> & result, const std::string & prefix, const cli_node_ptr & next) {
	std::string space;
	size_t space_sz = prefix.length() + 1;
	space.reserve(space_sz);
	for (size_t i = 0; i < space_sz; i++)
		space.push_back(' ');
	bool first = true;
	for (auto & entry : next->help()) {
		if (first) {
			first = false;
			result.push_back(prefix + ' ' + entry);
		} else {
			result.push_back(space + entry);
		}
	}
}

std::vector<std::string> choice_cli_node::help() {
	std::vector<std::string> result;
	for (auto & node : children)
		generate_help(result, node.first, node.second);
	return result;
}

void word_cli_form::fill(const std::map<std::string, std::string> & values) {
	if (values.size() != 1)
		throw cli_form_error("word form requires only one field");
	auto it = values.find("word");
	if (it != values.end())
		word = it->second;
	else
		throw cli_form_error("can't find \"word\" field");
}

void word_cli_node::process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) {
	size_t word_sz = find_space(str);
	if (!word_sz)
		throw cli_form_error("empty word field");
	forms.emplace(name, std::make_unique<word_cli_form>(std::string(str, word_sz)));
	str += word_sz;
	skip_space(str);
	next->process(str, forms);
}

std::vector<std::string> word_cli_node::help() {
	std::vector<std::string> result;
	std::string me = '<' + name + '>';
	generate_help(result, me, next);
	return result;
}

void form_base_cli_node::process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) {
	std::map<std::string, std::string> fields;
	while (true) {
		size_t tmp = measure_word(str);
		std::string key(str, tmp);
		str += tmp;
		if (fields.find(key) != fields.end())
			throw cli_form_error("option \""+ key + "\" already set");
		if (*str == '=') {
			str++;
			std::string value;
			if (*str == '"') {
				str++;
				tmp = find_escaped_end(str);
				value = unescape(str, tmp - 1);
			} else {
				tmp = measure_word(str);
				value = std::string(str, tmp);
			}
			str += tmp;
			fields.emplace(key, value);
		} else {
			fields.emplace(key, "true");
		}
		bool do_break = *str != ',';
		if (!do_break)
			str++;
		skip_space(str);
		if (do_break) break;
	}
	forms.emplace(name, fill_form(fields));
	next->process(str, forms);
}

std::vector<std::string> form_base_cli_node::help() {
	std::vector<std::string> result;
	std::string me;
	bool first = true;
	for (auto & field : fields()) {
		if (!first)
			me += ',';
		else
			first = false;
		me += field.first + '=' + field.second;
	}
	generate_help(result, me, next);
	return result;
}

cli_form_ptr form_cli_node::fill_form(const std::map<std::string, std::string> & values) {
	cli_form_ptr form = factory();
	form->fill(values);
	return form;
}

std::map<std::string, std::string> form_cli_node::fields() {
	return hints;
}

void option_cli_node::process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) {
	size_t word_sz = find_space(str);
	std::string option(str, word_sz);
	str += word_sz;
	skip_space(str);
	auto it = options.find(option);
	if (it != options.end()) {
		forms.emplace(name, std::make_unique<word_cli_form>(option));
	} else
		throw cli_choice_error("no option \"" + option + "\"");
	next->process(str, forms);
}

std::vector<std::string> option_cli_node::help() {
	if (options.empty())
		return {};
	auto it = options.begin();
	std::string collect = *it++;
	for (auto end = options.end(); it != end; it++) {
		collect += '|' + *it;
	}
	std::vector<std::string> result;
	generate_help(result, collect, next);
	return result;
}

void action_base_cli_node::process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) {
	if (*str != '\0')
		throw cli_error("command endian expected");
	action(forms);
}

std::vector<std::string> action_base_cli_node::help() {
	return { ": " + description() };
}

std::string action_cli_node::description() {
	return message;
}

void action_cli_node::action(std::unordered_map<std::string, cli_form_ptr> & forms) {
	func->operator()(forms);
}

} // namespace mcshub
