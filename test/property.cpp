#include "property.h"
#include <vector>
#include "test.h"

int main() {
    using namespace mcshub;
    p<int> pp = 47;
    bool was_called = false;
    auto it = pp.listen([&was_called](int old, int new_v) {
        assert_equals(47, old);
        assert_equals(567, new_v);
        was_called = true;
    });
    assert_equals(47, pp);
    pp = 567;
    assert_equals(true, was_called);
    assert_equals(567, pp);
    was_called = false;
    pp.ignore(it);
    pp = 37;
    assert_equals(false, was_called);
    /////////////////
    const p<std::string> strings = "old";
    strings.listen([&was_called](const std::string & old, const std::string & new_val) {
        was_called = true;
    });
    p<std::string> copy = strings;
    copy = std::string("new_val");
    assert_equals(true, was_called);
    return 0;
}
