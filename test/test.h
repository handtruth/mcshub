#include "log.h"
#include <string>
#include <exception>

namespace mcshub {
namespace test {

class assertion_error : std::runtime_error {
public:
    assertion_error(const std::string & message) : std::runtime_error(message) {}
};

using std::to_string;

std::string to_string(const std::string & str) {
    return str;
}

struct test_struct {
    stdout_log std_log;
    int success;
    test_struct(log_level lvl) : std_log(lvl), success(0) {
        log = &std_log;
    }
    template <typename first, typename second>
    void assert_eq(const first & expect, const second & actual, const char * file, std::size_t line) {
        if (actual != expect) {
            std::string message = std::string(file) + ":" + std::to_string(line) + ": assertion failed (\"" + to_string(expect) +
            "\" expected, got \"" + to_string(actual) + "\")";
            log->error(message);
            throw assertion_error(message);
        } else
            success++;
    }
    ~test_struct() {
        log->info("Success: " + std::to_string(success) + " assertions passed!");
    }
} test(log_level::debug);

#define assert_equals(expect, actual) \
        ::mcshub::test::test.assert_eq(expect, actual, __FILE__, __LINE__)

}
}
