#include <fstream>

#include <ekutils/log.hpp>

#include "response_props.hpp"
#include "strparts.hpp"

#include "test.hpp"

test {
	using namespace mcshub;

	ekutils::log = new ekutils::stdout_log(ekutils::log_level::debug);

	pakets::handshake hs;
	hs.address() = "nowhere.com";
	hs.port() = 25555;
	img_vars i_vars;
	std::ofstream("img_vars_f.txt") << "text";
	vars_manager man = make_vars_manager(main_vars, env_vars, hs, i_vars);
	{
		strparts s;
		s.append("Hello");
		s.append(std::string(" "));
		s.append(std::string_view("world", 5));
		assert_equals("Hello world", s.src());
		assert_equals("Hello world", s.resolve(man));
		s.simplify();
		assert_equals("Hello world", s.resolve(man));
	} {
		strparts s("clearly nothing to parse and concatenate");
		assert_true(s.simple());
		assert_equals("clearly nothing to parse and concatenate", s.resolve(man));
	} {
		strparts s("lol ${ hs:port } kek");
		assert_false(s.simple());
		assert_equals("lol 25555 kek", s.resolve(man));
	} {
		strparts s("kek $hs:address*lol");
		assert_false(s.simple());
		assert_equals("kek nowhere.com*lol", s.resolve(man));
	} {
		strparts s("zero $$moro");
		assert_equals("zero $moro", s.resolve(man));
	} {
		strparts s("lava $$$hs:port${hs:state}${}$ ${   hs:address} $");
		assert_false(s.simple());
		strparts p = s.resolve_partial(man);
		assert_equals("lava $255550{ NULL }$ nowhere.com $", p.src());
		empty_vars empty;
		assert_equals("lava $255550{ NULL }$ nowhere.com $", p.resolve(empty));
	} {
		strparts s("kaka $uuid makaka");
		assert_false(s.simple());
		std::cout << s.resolve(man) << std::endl;
	} {
		strparts s("image ${img:/img_vars_f.txt} end");
		assert_false(s.simple());
		assert_equals("image data:image/png;base64,dGV4dA== end", s.resolve(man));
	} {
		strparts s("no such $lol:kek namespace");
		assert_false(s.simple());
		assert_equals("no such ${lol:kek} namespace", s.resolve(man));
	} {
		strparts s("no such $$lol:kek namespace");
		assert_true(s.simple());
		assert_equals("no such $lol:kek namespace", s.resolve(man));
	}
}
