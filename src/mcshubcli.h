#ifndef _MCSHUBCLI_HEAD
#define _MCSHUBCLI_HEAD

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>
#include <stdexcept>
#include <functional>
#include <vector>

#include "delegate.h"

namespace mcshub {

class cli_node;
typedef std::shared_ptr<cli_node> cli_node_ptr;

struct cli_error : public std::runtime_error {
	cli_error(const std::string & message) : std::runtime_error(message) {}
	cli_error(const char * message) : std::runtime_error(message) {}
};

struct cli_form {
public:
	virtual ~cli_form();
	virtual void fill(const std::map<std::string, std::string> & values) = 0;
};

typedef std::unique_ptr<cli_form> cli_form_ptr;

class cli_node {
public:
	virtual void process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) = 0;
	virtual std::vector<std::string> help() = 0;
	virtual ~cli_node();
};

class choice_cli_node;
class word_cli_node;
class form_cli_node;
class option_cli_node;
class action_cli_node;

class chain_cli_node : public cli_node {
protected:
	cli_node_ptr next;
	chain_cli_node() {}
public:
	void chain(const cli_node_ptr & node) {
		next = node;
	}
	std::shared_ptr<choice_cli_node> choice();
	std::shared_ptr<word_cli_node> word(const std::string & form);
	std::shared_ptr<form_cli_node> form(
		const std::string & form,
		const std::function<cli_form_ptr(void)> & factory,
		const std::map<std::string, std::string> & fields);
	std::shared_ptr<option_cli_node> option(const std::string & form, const std::unordered_set<std::string> & options);
	template <typename F>
	std::shared_ptr<action_cli_node> action(F action, const std::string & description);
};

struct cli_choice_error : public cli_error {
	cli_choice_error(const std::string & message) : cli_error(message) {}
};

class choice_cli_node final : public cli_node {
	std::unordered_map<std::string, cli_node_ptr> children;
public:
	void add(const std::string & name, const cli_node_ptr & node);
	std::shared_ptr<choice_cli_node> choice(const std::string & name);
	std::shared_ptr<word_cli_node> word(const std::string & name, const std::string & form);
	std::shared_ptr<form_cli_node> form(
		const std::string & name,
		const std::string & form,
		const std::function<cli_form_ptr(void)> & factory,
		const std::map<std::string, std::string> & fields);
	std::shared_ptr<option_cli_node> option(
		const std::string & name,
		const std::string & form,
		const std::unordered_set<std::string> & options);
	template <typename F>
	std::shared_ptr<action_cli_node> action(const std::string & name, F act, const std::string & description);
	virtual void process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) override;
	virtual std::vector<std::string> help() override;
	const std::unordered_map<std::string, cli_node_ptr> options() const {
		return children;
	}
};

struct word_cli_form : public cli_form {
	std::string word;
	word_cli_form() = default;
	word_cli_form(std::string init) : word(init) {}
	virtual void fill(const std::map<std::string, std::string> & values) override;
};

inline std::string form_as_word(std::unordered_map<std::string, cli_form_ptr> & forms, const std::string & word) {
	return dynamic_cast<word_cli_form &>(*forms[word]).word;
}

template <typename T>
inline T & get_form(std::unordered_map<std::string, cli_form_ptr> & forms, const std::string & name) {
	return dynamic_cast<T &>(*forms[name]);
}

class word_cli_node final : public chain_cli_node {
	std::string name;
public:
	word_cli_node(const std::string & form) : name(form) {}
	virtual void process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) override;
	virtual std::vector<std::string> help() override;
};

struct cli_form_error : public cli_error {
	cli_form_error(const std::string & message) : cli_error(message) {}
	cli_form_error(const char * message) : cli_error(message) {}
};

class form_base_cli_node : public chain_cli_node {
protected:
	std::string name;
	form_base_cli_node(const std::string & form) : name(form) {}
public:
	virtual void process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) override;
	virtual std::vector<std::string> help() override;
	virtual cli_form_ptr fill_form(const std::map<std::string, std::string> & values) = 0;
	virtual std::map<std::string, std::string> fields() = 0;
};

class form_cli_node : public form_base_cli_node {
	std::function<cli_form_ptr(void)> factory;
	std::map<std::string, std::string> hints;
public:
	form_cli_node(const std::string & form, const std::function<cli_form_ptr(void)> & creator,
		const std::map<std::string, std::string> & fields) :
			form_base_cli_node(form),
			factory(creator), hints(fields) {}
	virtual cli_form_ptr fill_form(const std::map<std::string, std::string> & values) override;
	virtual std::map<std::string, std::string> fields() override;
};

class option_cli_node : public chain_cli_node {
	std::string name;
	std::unordered_set<std::string> options;
public:
	option_cli_node(const std::string & form, const std::unordered_set<std::string> & opts) :
			name(form), options(opts) {}
	virtual void process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) override;
	virtual std::vector<std::string> help() override;
};

class action_base_cli_node : public cli_node {
public:
	virtual void process(const char * str, std::unordered_map<std::string, cli_form_ptr> & forms) override;
	virtual std::vector<std::string> help() override;
	virtual std::string description() = 0;
protected:
	action_base_cli_node() {}
	virtual void action(std::unordered_map<std::string, cli_form_ptr> & forms) = 0;
};

class action_cli_node : public action_base_cli_node {
	std::unique_ptr<delegate_base<void(std::unordered_map<std::string, cli_form_ptr> &)>> func;
	std::string message;
public:
	template <typename F>
	action_cli_node(F act, const std::string & description) :
		func(std::make_unique<delegate_t<F, void(std::unordered_map<std::string, cli_form_ptr> &)>>(act)),
		message(description) {}
	virtual std::string description() override;
protected:
	virtual void action(std::unordered_map<std::string, cli_form_ptr> & forms) override;
};

template <typename F>
std::shared_ptr<action_cli_node> chain_cli_node::action(F act, const std::string & description) {
	auto ptr = std::make_shared<action_cli_node>(act, description);
	chain(ptr);
	return ptr;
}

template <typename F>
std::shared_ptr<action_cli_node> choice_cli_node::action(const std::string & name, F act, const std::string & description) {
	auto ptr = std::make_shared<action_cli_node>(act, description);
	add(name, ptr);
	return ptr;
}

} // namespace mcshub

#endif // _MCSHUBCLI_HEAD
