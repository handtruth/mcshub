#ifndef RESOURCES_EXT_HEAD_PQQAXOTN
#define RESOURCES_EXT_HEAD_PQQAXOTN

#include <string_view>
#include <string>
#include <array>
#include <cinttypes>

namespace res {

	template <std::size_t N>
	std::string_view view_of(const std::array<std::uint8_t, N> & data) {
		return std::string_view(reinterpret_cast<const char *>(data.data()), data.size());
	}
	
	template <std::size_t N>
	std::string str_of(const std::array<std::uint8_t, N> & data) {
		return std::string(view_of(data));
	}

}

#endif // RESOURCES_EXT_HEAD_PQQAXOTN
