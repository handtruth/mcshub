#include "test.h"

#include "mutex_atomic.h"
#include "putil.h"

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
	return 0;
}
