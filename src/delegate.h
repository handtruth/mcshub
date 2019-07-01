#ifndef _EVENT_HANDLER
#define _EVENT_HANDLER

namespace mcshub {

template <typename F>
class delegate_base;

template <typename R, typename ...Args>
class delegate_base<R(Args...)> {
public:
	virtual R operator()(Args... args) = 0;
	virtual ~delegate_base() {}
};

template <typename C, typename F>
class delegate_t;

template <typename C, typename R, typename ...Args>
class delegate_t<C, R(Args...)> : public delegate_base<R(Args...)> {
	C callable;
public:
	delegate_t(C call) : callable(call) {}
	virtual R operator()(Args... args) override {
		return callable(args...);
	}
};

} // mcshub

#endif // _EVENT_HANDLER
