#include "test.h"
#include "response_props.h"
#include "uuid.h"
#include <cstring>

const char * sneacky = "666 ${ lol } 777";

int main() {
	using namespace mcshub;
	int l = find_closing_bracket<'{'>("{ 894h {fde } gfd}  }");
	assert_equals(18, l);
	const char * stringa = "              lol kek chburek                ";
	mcshub::size_t stringa_len = std::strlen(stringa);
	trim_string(stringa, stringa_len);
	assert_equals(std::string("lol kek chburek"), std::string(stringa, stringa_len));
	pakets::handshake hs;
	hs.address() = "nowhere.com";
	hs.port() = 25555;
	vars_manager man = make_vars_manager(main_vars, env_vars, hs);
	std::string c1 = man.resolve("lol ${ hs:port } kek");
	assert_equals("lol 25555 kek", c1);
	std::string c2 = man.resolve("kek $hs:address.lol");
	assert_equals("kek nowhere.com.lol", c2);
	std::string c3 = man.resolve("zero $$moro");
	assert_equals("zero $moro", c3);
	std::string c4 = man.resolve("lava $$$hs:port${hs:state}${}$ ${   hs:address} $");
	assert_equals("lava $255550{ NULL }$ nowhere.com $", c4);
	std::string c5 = man.resolve("kaka $uuid makaka");
	log->debug(c5);
	srand(65443);
	for (int i = 0; i < 10; i++) {
		std::string id = uuid::random();
		log->debug("uuid example: " + id);
	}
	return 0;
}
