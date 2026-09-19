#ifndef JOPP_SEQUENCE_CONTAINER_HPP
#define JOPP_SEQUENCE_CONTAINER_HPP

#include <concepts>

namespace jopp2
{
	template<class T>
	concept sequence_container = requires(T& obj){
		{obj.back()};
		{obj.push_back(std::declval<typename T::value_type>())};
		{obj.emplace_back(std::declval<typename T::value_type>())};
		{obj.empty()} -> std::same_as<bool>;
	};
}

#endif
