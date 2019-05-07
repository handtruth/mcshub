#include "test.h"

#include "mutex_atomic.h"
#include "putil.h"

#include <memory>

int ohh = 0;

struct base_t {
	~base_t() {};
};

struct hehoh : public base_t {
	~hehoh() {
		ohh = 10;
	}
};

int main() {
	using namespace mcshub;
	matomic<int> m;
	m = 10;
	assert_equals(10, int(m));
	bool was_called = false;
	{
		finnaly({
			was_called = true;
		});
		assert_equals(false, was_called);
	}

	assert_equals(true, was_called);
	{
		std::shared_ptr<base_t> base = std::make_shared<hehoh>();
	}
	assert_equals(10, ohh);
	return 0;
}
