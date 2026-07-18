#ifndef JOPP_UPDATER_HPP
#define JOPP_UPDATER_HPP

#include <type_traits>
#include <cstddef>
#include <tuple>
#include <cstdio>
#include <source_location>

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
		#define UPDATE_CALLBACK [[gnu::fastcall]]
	#else
		#define UPDATE_CALLBACK
	#endif

	template<class T>
	inline constexpr auto pass_by_value_v = pass_by_value<T>::value;

	template <class T>
	using update_param_t = std::conditional_t<
		pass_by_value_v<T>,
		T,
		std::conditional_t<std::is_trivially_copyable_v<T>, T const&, T&&>
	>;

	template<class T>
	inline constexpr decltype(auto) maybe_move(T&& src)
	{
		if constexpr(std::is_reference_v<T>)
		{
			if constexpr(!std::is_const_v<std::remove_reference_t<T>>)
			{ return std::move(src); }
			else
			{ return src; }
		}
		else
		{ return src; }
	}

	template <class T>
	using query_param_t = std::conditional_t<pass_by_value_v<T>, T, T const&>;

	template<template<class> class UpdateResultType, class Sink, class T>
	using update_func_t = UpdateResultType<T> (*)(Sink&, update_param_t<T>) UPDATE_CALLBACK;

	template<class UpdateTraits, template<class> class UpdateResultType, class Sink, class... Types>
	concept update_traits = (requires(update_func_t<UpdateResultType, Sink, Types>& cb)
	{
		{ cb = &UpdateTraits::update };
	}
	&& ...);

	template<template<class> class UpdateResultType, class... Types>
	class updater
	{
	public:
		constexpr updater() = default;
		template<class Sink, update_traits<UpdateResultType, Sink, Types...> UpdateTraits>
		constexpr explicit updater(Sink& target, std::type_identity<UpdateTraits>, char const* origin = std::source_location::current().function_name()):
				m_handle{&target},
				m_vtable{&s_vtable<Sink, UpdateTraits>},
				m_origin{origin}
		{}

		template<class SourceValue>
		requires(
			!std::is_lvalue_reference_v<SourceValue> ||
			pass_by_value_v<std::remove_cvref_t<SourceValue>>
		)
		[[nodiscard]] constexpr auto update_with(SourceValue&& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			return std::get<update_callback_t<raw_type>>(*m_vtable)(m_handle, std::forward<SourceValue>(value));
		}

		template<class SourceValue>
		requires(!pass_by_value_v<std::remove_cvref_t<SourceValue>>)
		[[nodiscard]] constexpr auto update_with(SourceValue const& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			if constexpr(std::is_trivially_copyable_v<std::remove_cvref_t<SourceValue>>)
			{ return std::get<update_callback_t<raw_type>>(*m_vtable)(m_handle, value); }
			else
			{ return std::get<update_callback_t<raw_type>>(*m_vtable)(m_handle, raw_type{value}); }
		}

		constexpr operator bool() const
		{ return m_handle != nullptr; }

		char const* origin() const
		{ return m_origin; }

	private:
		template<class T>
		using update_callback_t = UpdateResultType<T> (*)(void*, update_param_t<T>) UPDATE_CALLBACK;

		using vtable = std::tuple<update_callback_t<Types>...>;

		void* m_handle{nullptr};
		vtable const* m_vtable{nullptr};
		char const* m_origin{nullptr};

		template<class Sink, class UpdateTraits>
		static constexpr vtable s_vtable{
			[](void* target, update_param_t<Types> value) UPDATE_CALLBACK {
				if constexpr(pass_by_value_v<Types> || std::is_trivially_copyable_v<Types>)
				{ return UpdateTraits::update(*static_cast<Sink*>(target), value); }
				else
				{ return UpdateTraits::update(*static_cast<Sink*>(target), std::move(value)); }
			}...
		};
	};
}

#endif