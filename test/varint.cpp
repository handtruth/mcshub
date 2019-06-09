#include "test.h"
#include "paket.h"

#include <limits>

const mcshub::size_t N = 1000;

int main() {
    using namespace mcshub;
    byte_t data[N];
    std::int32_t value;
    assert_equals(1, write_varint(data, N, 25));
    assert_equals(1, read_varint(data, N, value));
    assert_equals(25, value);

    assert_equals(5, write_varint(data, N, std::numeric_limits<std::int32_t>::max()));
    assert_equals(2, write_varint(data, N, 200));

    assert_equals(2, read_varint(data, N, value));
    assert_equals(200, value);

    assert_equals(mcshub::size_t(1), size_varint(25));
    assert_equals(mcshub::size_t(2), size_varint(200));
    assert_equals(mcshub::size_t(5), size_varint(std::numeric_limits<std::int32_t>::max()));

    return 0;
}
