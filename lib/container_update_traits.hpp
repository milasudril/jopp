#ifndef JOPP_CONTAINER_UPDATE_TRAITS_HPP
#define JOPP_CONTAINER_UPDATE_TRAITS_HPP

#include "./exception.hpp"
#include "./node_visitor_adaptor.hpp"
#include "./sequence_container.hpp"
#include <type_traits>

namespace jopp2
{
	template<class Container>
	class container_update_traits
	{
	public:
		using output_value_type = Container::mapped_type;
		using container_key_type = Container::key_type;

		template<class Rhs>
		[[gnu::always_inline]] [[noreturn]] static output_value_type* update(
			Container& /*unused*/,
			Rhs&& /*unused*/
		)
		{ raise_internal_error("Objects can only update keys"); }

		template<class Rhs>
		requires (
			 instance_of<std::remove_cvref_t<Rhs>, key_to_clone>
		&& std::is_constructible_v<container_key_type, Rhs>
		)
		[[gnu::always_inline]] static output_value_type* update(
			Container& object,
			Rhs&& key
		)
		{
			auto const insertion_pair = object.insert(
				std::pair{
					std::forward_like<Rhs>(std::forward<Rhs>(key).value),
					output_value_type{}
				}
			);
			return &insertion_pair.first->second;
		}
	};

	template<sequence_container Container>
	class container_update_traits<Container>
	{
	public:
		using output_value_type = Container::value_type;

		template<class Rhs>
		static auto update(Container&, Rhs&&) = delete;

		template<class Rhs>
		requires(std::is_constructible_v<output_value_type, Rhs>)
		[[gnu::always_inline]] static output_value_type* update(Container& lhs, Rhs&& rhs)
		{
			lhs.emplace_back(std::forward<Rhs>(rhs));
			return &lhs.back();
		}

		template<class Rhs>
		requires instance_of<std::remove_cvref_t<Rhs> ,key_to_clone>
		[[gnu::always_inline]] [[noreturn]] static output_value_type* update(
			Container& /*unused*/,
			Rhs&& /*unsed*/
		)
		{ raise_internal_error("Cannot store a key in a sequence container"); }
	};


}

#endif
