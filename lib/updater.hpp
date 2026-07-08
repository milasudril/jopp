#ifndef JOPP_UPDATER_HPP
#define JOPP_UPDATER_HPP

#include <type_traits>
#include <cstddef>
#include <tuple>

namespace jopp2
{

	template <class T>
	struct is_passed_in_register
	{
		// 1. Must be trivial for the purpose of calls (per C++ ABI)
		static constexpr bool is_trivial_abi = std::is_trivially_copyable_v<T>;

		// 2. Size must be between 1 and 16 bytes (or be an empty struct/fundamental)
		static constexpr bool valid_size = (sizeof(T) <= 16);

		// 3. Must not have unaligned layout
		// If the alignment of the type is less than the standard alignment of its size,
		// or if it lacks natural alignment matching its layout, the ABI rejects it.
		static constexpr bool valid_alignment = (alignof(T) >= 1) &&
				(sizeof(T) <= 8 ? (alignof(T) >= sizeof(T) || sizeof(T) == 0) : (alignof(T) >= 8));

		static constexpr bool value = is_trivial_abi && valid_size && valid_alignment;
	};

	template<class T>
	inline constexpr auto is_passed_in_register_v = is_passed_in_register<T>::value;

	template <class T>
	using using_optimal_sink_t = std::conditional_t<is_passed_in_register_v<T>, T, T&&>;

	template<class... Types>
	class updater
	{
	public:
		template<class Sink, class UpdateTraits>
		explicit updater(Sink& target, std::type_identity<UpdateTraits>):
			m_handle{&target},
			m_vtable{&s_vtable<Sink, UpdateTraits>}
		{}

		template<class SourceValue>
		requires(
			!std::is_lvalue_reference_v<SourceValue> ||
			is_passed_in_register_v<std::remove_cvref_t<SourceValue>>
		)
		void update(SourceValue&& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			std::get<callback_type<raw_type>>(*m_vtable)(m_handle, std::forward<SourceValue>(value));
		}

		template<class SourceValue>
		requires(!is_passed_in_register_v<std::remove_cvref_t<SourceValue>>)
		void update(SourceValue const& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			std::get<callback_type<raw_type>>(*m_vtable)(m_handle, raw_type{value});
		}

	private:
		template<class T>
		using callback_type = void (*)(void*, using_optimal_sink_t<T>);
		using vtable = std::tuple<callback_type<Types>...>;

		void* m_handle;
		vtable const* m_vtable;

		template<class Sink, class UpdateTraits>
		static constexpr vtable s_vtable{
			[](void* target, using_optimal_sink_t<Types> value) {
				if constexpr(is_passed_in_register_v<Types>)
				{ UpdateTraits::update(*static_cast<Sink*>(target), value); }
				else
				{ UpdateTraits::update(*static_cast<Sink*>(target), std::move(value)); }
			}...
		};
	};
}

#endif