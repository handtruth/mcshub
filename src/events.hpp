#ifndef EVENTS_HEAD_QCDVPFDBFTBG
#define EVENTS_HEAD_QCDVPFDBFTBG

#include <ekutils/event_d.hpp>

namespace mcshub {

template <typename E>
struct events_base final : public ekutils::event_d {
	std::unique_ptr<E> read() {
		std::unique_ptr<E> ptr;
		ptr.reset(reinterpret_cast<E *>(event_d::read()));
		return ptr;
	}
	void write(std::unique_ptr<E> && ev) {
		event_d::write(reinterpret_cast<std::uint64_t>(ev.release()));
	}
	template <typename T>
	void write_event(const T & ev) {
		write(std::make_unique<T>(ev));
	}
	template <typename T>
	void write_event(T && ev) {
		write(std::make_unique<T>(std::move(ev)));
	}
	events_base() : event_d(std::int32_t(0)) {}
};

} // namespace mcshub

#endif // EVENTS_HEAD_QCDVPFDBFTBG
