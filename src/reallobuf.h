#ifndef _REALLOBUF_HEAD
#define _REALLOBUF_HEAD

#include "primitives.h"

namespace mcshub {

class reallobuf final {
private:
    size_t sz;
    byte_t * bytes;
public:
    const size_t k = 1;
    explicit reallobuf(size_t initial = 8);
    reallobuf(const reallobuf & other);
    reallobuf(reallobuf && other);
    size_t probe(size_t amount);
    void move(size_t i);
    size_t size() const noexcept {
        return sz;
    }
    byte_t * data() noexcept {
        return bytes;
    }
    const byte_t * data() const noexcept {
        return bytes;
    }
    ~reallobuf();
};

} // mcshub

#endif // _REALLOBUF_HEAD
