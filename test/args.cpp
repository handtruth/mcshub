#include "test.hpp"

#include "prog_args.hpp"

test {
    using namespace mcshub;
    using namespace std;
    using namespace std::literals;
    arguments_t args;
    const char * strings[5] = { "-m", "--confname=lol.yml", "--default-dir", "kek", "--port=255" };
    args.parse(5, strings);
    assert_equals(std::string("lol.yml"), args.confname);
    assert_equals(true, args.mcsman);
    assert_equals("kek", args.default_srv_dir);
    assert_equals(255, args.port);

    assert_equals(false, args.install);
}
