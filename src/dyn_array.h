
#ifndef _DYN_ARRAY_HEAD
#define _DYN_ARRAY_HEAD

#include <cinttypes>

namespace mcshub {

typedef std::uint8_t byte_t;
typedef std::uint32_t size_type;

template <class T>
class dyn_array {
public:
	typedef T element_type;

private:
	T * data;
	size_type count;

	void throw_out_of_range(size_type index) const {
		throw std::out_of_range(std::string("dyn array index (")
			+ std::to_string(index) + ") out of range [0.." + std::to_string(count) + "]");
	}

public:
	dyn_array(size_type length) {
		data = new T[count = length];
	}

	typename T & at(size_type index) {
		if (index >= count)
			throw_out_of_range(index);
		return data[index];
	}
	typename T at(size_type index) const {
		if (index >= count)
			throw_out_of_range(index);
		return data[index];
	}
	typename T & operator[](size_type index) {
		return bytes[index];
	}
	typename T operator[](size_type index) const {
		return data[index];
	}
	size_type size() const noexcept {
		return count;
	}
	~dyn_array() {
		delete[] data;
	}
};

}

#endif // _DYN_ARRAY_HEAD
