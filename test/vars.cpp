#include "test.hpp"
#include "response_props.hpp"
#include <ekutils/uuid.hpp>
#include <cstring>
#include <ekutils/log.hpp>

const char * sneacky = "666 ${ lol } 777";

test {
	using namespace mcshub;
	int l = ekutils::find_closing_bracket<'{'>("{ 894h {fde } gfd}  }");
	assert_equals(18, l);
	const char * stringa = "              lol kek chburek                ";
	std::size_t stringa_len = std::strlen(stringa);
	ekutils::trim_string(stringa, stringa_len);
	assert_equals(std::string("lol kek chburek"), std::string(stringa, stringa_len));
	pakets::handshake hs;
	hs.address() = "nowhere.com";
	hs.port() = 25555;
	img_vars i_vars;
	std::ofstream("img_vars_f.txt") << "text";
	vars_manager man = make_vars_manager(main_vars, env_vars, hs, i_vars);
	std::string c1 = man.resolve("lol ${ hs:port } kek");
	assert_equals("lol 25555 kek", c1);
	std::string c2 = man.resolve("kek $hs:address.lol");
	assert_equals("kek nowhere.com.lol", c2);
	std::string c3 = man.resolve("zero $$moro");
	assert_equals("zero $moro", c3);
	std::string c4 = man.resolve("lava $$$hs:port${hs:state}${}$ ${   hs:address} $");
	assert_equals("lava $255550{ NULL }$ nowhere.com $", c4);
	std::string c5 = man.resolve("kaka $uuid makaka");
	log_debug(c5);
	std::string c6 = man.resolve("image ${img:/img_vars_f.txt} end");
	assert_equals("image data:image/png;base64,dGV4dA== end", c6);
	srand(65443);
	for (int i = 0; i < 10; i++) {
		std::string id = ekutils::uuid::random();
		log_debug("uuid example: " + id);
	}
}
