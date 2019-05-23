#include "test.h"

#include "prog_args.h"

int main() {
    using namespace mcshub;
    using namespace std;
    using namespace std::literals;
    arguments_t args;
    const char * strings[5] = { "-m", "--confname=lol.yml", "--default_dir", "kek", "--port=255" };
    args.parse(5, strings);
    assert_equals(std::string("lol.yml"), args.confname);
    assert_equals(true, args.mcsman);
    assert_equals("kek", args.default_srv_dir);
    assert_equals(255, args.port);

    assert_equals(false, args.install);
    return 0;
}
