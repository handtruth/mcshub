#include "test.hpp"

#include "mcshub_arguments.hpp"

test {
    using namespace mcshub;
    using namespace std;
    using namespace std::literals;
    arguments_t args;
    const char * strings[5] = { "mcshub", "-m", "--confname=lol.yml", "--default-dir", "kek" };
    args.parse(5, strings);
    assert_equals(std::string("lol.yml"), args.confname);
    assert_equals(true, args.mcsman);
    assert_equals("kek", args.default_srv_dir);
    assert_equals(false, args.install);
    assert_true(args.positional.empty());
    log_debug(args.build_help("mcshub"));
}
