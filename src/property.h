#ifndef _PROPERTY_HEAD
#define _PROPERTY_HEAD

#include <list>
#include <memory>
#include <iterator>

#include "delegate.h"

namespace mcshub {

template <typename T>
//typedef int T;
class property_t {
public:
	typedef delegate_base<void(const T &, const T &)> action_t;
	typedef std::list<std::unique_ptr<action_t>> callers;
private:

	mutable std::shared_ptr<callers> listeners;

	T value;
	void notify(const T & old_value, const T & new_value) {
		for (std::unique_ptr<action_t> & listener : *listeners)
			(*listener)(old_value, new_value);
	}
public:
	property_t() : listeners(std::make_shared<callers>()) {}
	property_t(const T & arg) : listeners(std::make_shared<callers>()), value(arg) {}
	property_t(T && arg) : listeners(std::make_shared<callers>()), value(std::move(arg)) {}
	template <typename ...Args>
	property_t(Args &... args) : listeners(std::make_shared<callers>()), value(args...) {}
	operator const T &() const {
		return value;
	}
	const T & operator*() const {
		return value;
	}
	property_t & operator=(const T & other) {
		notify(value, other);
		value = other;
	}
	property_t & operator=(T && other) {
		notify(value, other);
		value = std::move(other);
		return *this;
	}
	template <typename F>
	typename callers::const_iterator listen(F listener) const {
		listeners->emplace_front(std::make_unique<delegate_t<F, void(const T &, const T &)>>(listener));
		return listeners->cbegin();
	}
	void ignore(typename callers::const_iterator & iterator) const {
		listeners->erase(iterator);
	}
};

template <typename T>
using p = property_t<T>;

} // mcshub

#endif // _PROPERTY_HEAD
