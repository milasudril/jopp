#ifndef JOPP_UPDATER_HPP
#define JOPP_UPDATER_HPP

#include <type_traits>
#include <cstddef>
#include <tuple>

namespace jopp2
{
	template <class T>
	struct pass_by_value
	{
		static constexpr bool is_trivial_abi = std::is_trivially_copyable_v<T> || std::is_reference_v<T>;

	#ifndef __i386__
		static constexpr bool platform_gate = (sizeof(T) <= 2*sizeof(void*));
	#else
		static constexpr bool platform_gate = std::is_fundamental_v<T>
			|| std::is_pointer_v<T>
			|| std::is_reference_v<T>
			|| std::is_enum_v<T>;
	#endif
		static constexpr bool value = is_trivial_abi && platform_gate;
	};

	#ifdef __i386__
		#define THISCALL [[gnu::thiscall]]
	#else
		#define THISCALL
	#endif

	template<class T>
	inline constexpr auto pass_by_value_v = pass_by_value<T>::value;

	template <class T>
	using using_optimal_sink_t = std::conditional_t<
		pass_by_value_v<T>,
		T,
		std::conditional_t<std::is_trivially_copyable_v<T>, T const&, T&&>
	>;

	template <class T>
	using using_optimal_param_t = std::conditional_t<pass_by_value_v<T>, T, T const&>;

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
			pass_by_value_v<std::remove_cvref_t<SourceValue>>
		)
		THISCALL void update_with(SourceValue&& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			std::get<callback_type<raw_type>>(*m_vtable)(m_handle, std::forward<SourceValue>(value));
		}

		template<class SourceValue>
		requires(!pass_by_value_v<std::remove_cvref_t<SourceValue>>)
		THISCALL void update_with(SourceValue const& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			if constexpr(std::is_trivially_copyable_v<std::remove_cvref_t<SourceValue>>)
			{ std::get<callback_type<raw_type>>(*m_vtable)(m_handle, value); }
			else
			{ std::get<callback_type<raw_type>>(*m_vtable)(m_handle, raw_type{value}); }
		}

	private:
		template<class T>
		using callback_type = void (*)(void*, using_optimal_sink_t<T>) THISCALL;
		using vtable = std::tuple<callback_type<Types>...>;

		void* m_handle;
		vtable const* m_vtable;

		template<class Sink, class UpdateTraits>
		static constexpr vtable s_vtable{
			[](void* target, using_optimal_sink_t<Types> value) THISCALL {
				if constexpr(pass_by_value_v<Types> || std::is_trivially_copyable_v<Types>)
				{ UpdateTraits::update(*static_cast<Sink*>(target), value); }
				else
				{ UpdateTraits::update(*static_cast<Sink*>(target), std::move(value)); }
			}...
		};
	};
}

#endif