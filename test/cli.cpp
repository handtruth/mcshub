#include "test.h"

#include "mcshubcli.h"

using namespace mcshub;

struct example_form : public cli_form {
	std::string fieldA;
	int fieldB = 0;

	static cli_form_ptr instance() {
		return std::make_unique<example_form>();
	}

	virtual void fill(const std::map<std::string, std::string> & values) override {
		for (auto & entry : values) {
			if (entry.first == "fieldA")
				fieldA = entry.second;
			else if (entry.first == "fieldB")
				fieldB = std::stoi(entry.second);
		}
	}
};

int main() {
	choice_cli_node root = choice_cli_node();
	root.word("test", "example")->action([](auto & forms) {

	}, "example situation with field");
	auto & next = *root.choice("do");
	next.action("say", [](auto & forms) {
		log->info("say command!");
	}, "say something");
	std::string test_var;
	example_form test_form;
	next.word("get", "var")->action([&test_var](auto & forms) {
		test_var = dynamic_cast<const word_cli_form &>(*forms["var"]).word;
		log->debug("get " + test_var);
	}, "get a var");
	next.form("form", "example", example_form::instance, { { "fieldA", "[name]" }, { "fieldB", "<number>" } })->
		action([&test_form](auto & forms) {
			test_form = dynamic_cast<example_form &>(*forms["example"]);
		}, "fills a form");
	std::string test_opt;
	next.option("optics", "opa", {
		"lol", "kek", "zopa"
	})->action([&test_opt](auto & forms) {
		test_opt = form_as_word(forms, "opa");
	}, "optins exaple");
	log->info("ROOT");
	for (auto & entry : root.help()) {
		log->debug(entry);
	}

	std::unordered_map<std::string, cli_form_ptr> forms;

	assert_equals(std::size_t(2), root.options().size());
	root.process("do get property", forms);
	assert_equals("property", test_var);
	root.process(R"(do form fieldB=23, fieldA="lol \"kek")", forms);
	assert_equals(23, test_form.fieldB);
	assert_equals("lol \"kek", test_form.fieldA);
	root.process("do optics zopa", forms);
	assert_equals("zopa", test_opt);
}
