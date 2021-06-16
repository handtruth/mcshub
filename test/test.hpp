#ifdef __TEST_HEAD
#   error "test.hpp header can't be included several times"
#else
#   define __TEST_HEAD
#endif

// LCOV_EXCL_START

#include <string>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <vector>
#include <cstdio>

#include <ekutils/log.hpp>

namespace std {
    inline string to_string(const std::string & str) {
        return str;
    }
}

namespace tests {

class assertion_error : public std::runtime_error {
public:
    explicit assertion_error(const std::string & message) : std::runtime_error(message) {}
};

struct {
    int success = 0;
    template <typename first, typename second>
    void assert_eq(const first & expect, const second & actual, const char * file, std::size_t line) {
        if (actual != expect) {
            std::string message = std::string(file) + ":" + std::to_string(line) + ": assertion failed (\"" + std::to_string(expect) +
            "\" expected, got \"" + std::to_string(actual) + "\")";
            throw assertion_error(message);
        } else
            success++;
    }
    template <typename first, typename second>
    void assert_ne(const first & expect, const second & actual, const char * file, std::size_t line) {
        if (actual == expect) {
            std::string message = std::string(file) + ":" + std::to_string(line) + ": assertion failed (\"" + std::to_string(expect) +
            "\" equals to \"" + std::to_string(actual) + "\")";
            throw assertion_error(message);
        } else
            success++;
    }
    template <typename E, typename F>
    void assert_ex_with(F fun, const char * file, std::size_t line) {
        try {
            fun();
        } catch (const E & e) {
            success++;
            return;
        }
        std::string message = std::string(file) + ":" + std::to_string(line) + ": no exception cought.";
        throw assertion_error(message);
    }
    template <typename F>
    void assert_ex(F fun, const char * file, std::size_t line) {
        try {
            fun();
        } catch (...) {
            success++;
            return;
        }
        std::string message = std::string(file) + ":" + std::to_string(line) + ": no exception cought.";
        throw assertion_error(message);
    }
    template <typename ...F>
    void assert_an(const char * file, std::size_t line, F... checks) {
        bool passed = 0;
        std::string collected_message;
        int count = sizeof...(checks);

        auto f = [&](const auto & operand) -> void {
            int prev_success = success;
            try {
                operand();
                passed = true;
            } catch (const assertion_error & e) {
                success = prev_success;
                collected_message = "\n\tclause #" + std::to_string(count) + ": " + e.what() + collected_message;
            }
            count--;
        };
        [](...){}((f(std::forward<F>(checks)), 0)...);
        if (!passed) {
            std::string message = std::string(file) + ":" + std::to_string(line) +
                ": no assertion passed." + collected_message;
            throw assertion_error(message);
        }
    }
    void assert_tr(bool value, const char * file, std::size_t line, const char * expression) {
        if (value) {
            success++;
        } else {
            std::string message = std::string(file) + ":" + std::to_string(line) + ": value (" + expression + ") was false.";
            std::cout << "test failed: " << message << std::endl;
            throw assertion_error(message);
        }
    }
    void assert_fa(bool value, const char * file, std::size_t line, const char * expression) {
        if (!value) {
            success++;
        } else {
            std::string message = std::string(file) + ":" + std::to_string(line) + ": value (" + expression + ") was true.";
            std::cout << "test failed: " << message << std::endl;
            throw assertion_error(message);
        }
    }
} assert;

#define test \
        void test_function()

#define assert_equals(expect, actual) \
        ::tests::assert.assert_eq(expect, actual, __FILE__, __LINE__)

#define assert_not_equals(expect, actual) \
        ::tests::assert.assert_ne(expect, actual, __FILE__, __LINE__)

#define assert_fails_with(E, fun) \
        ::tests::assert.assert_ex_with<E>([&]() fun, __FILE__, __LINE__)

#define assert_fails(fun) \
        ::tests::assert.assert_ex([&]() fun, __FILE__, __LINE__)

#define assert_any(...) \
        ::tests::assert.assert_an(__FILE__, __LINE__, [&]() __VA_ARGS__)

#define assert_or , [&]()

#define assert_true(value) \
        ::tests::assert.assert_tr(value, __FILE__, __LINE__, #value)

#define assert_false(value) \
        ::tests::assert.assert_fa(value, __FILE__, __LINE__, #value)

}

void test_function();

int main() {
    ekutils::log = new ekutils::stdout_log(ekutils::log_level::debug);
    std::cout << "entering test..." << std::endl;
    try {
        test_function();
        std::cout << "success: " << tests::assert.success << " assertions passed";
        return 0;
    } catch (const tests::assertion_error & e) {
        std::cout << "assertion error: " << e.what() << std::endl;
        return 2;
    } catch (const std::exception & e) {
        std::cout << "fatal error: " << typeid(e).name() << ": " << e.what() << std::endl;
        return 1;
    }
}

// LCOV_EXCL_STOP
