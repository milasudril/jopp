#ifndef JOPP_JSON_TYPES_HPP
#define JOPP_JSON_TYPES_HPP

#include <variant>
#include <map>
#include <vector>
#include <string>
#include <memory>

namespace jopp
{
	class array;
	class object;
	using number = double;
	using value = std::variant<
		std::unique_ptr<object>,
		std::unique_ptr<array>,
		number,
		bool,
		std::monostate>;
}

#endif