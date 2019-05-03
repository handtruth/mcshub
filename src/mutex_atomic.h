#ifndef _MUTEX_ATOMIC_HEAD
#define _MUTEX_ATOMIC_HEAD

#include <shared_mutex>

namespace mcshub {

template <typename T>
class matomic {
private:
	mutable std::shared_mutex mutex;
	T object;
public:
	matomic() {}
	matomic(const T & data) : object(data) {}
	matomic(T && data) : object(static_cast<T &&>(data)) {}
	void operator=(const T & data) {
		std::unique_lock lock(mutex);
		object = data;
	}
	void operator=(const T && data) {
		std::unique_lock lock(mutex);
		object = std::move(data);
	}
	operator T() const {
		std::shared_lock lock(mutex);
		return object;
	}
};

} // mcshub


#endif // _MUTEX_ATOMIC_HEAD
