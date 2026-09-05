#ifndef JOPP_VALUE_STORAGE_HPP
#define JOPP_VALUE_STORAGE_HPP

#include <type_traits>
#include <tuple>

namespace jopp2
{
	/**
	 * \brief Type trait to determine whether or not T should be passed by value
	 */
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
		/**
		 * \brief Controls which calling convention is used for entries in the value_storage vtable
		 */
		#define UPDATE_CALLBACK
	#endif

	/**
	 * \brief \see pass_by_value
	 */
	template<class T>
	inline constexpr auto pass_by_value_v = pass_by_value<T>::value;

	/**
	 * \brief Type to be used for the source value in update callbacks
	 */
	template <class T>
	using update_param_t = std::conditional_t<
		pass_by_value_v<T>,
		T,
		std::conditional_t<std::is_trivially_copyable_v<T>, T const&, T&&>
	>;

	/**
	 * \brief Helper function for forwarding src
	 */
	template<class T>
	[[gnu::always_inline]] inline constexpr decltype(auto) maybe_move(T&& src)
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

	/**
	 * \brief Type alias for update callbacks
	 * \tparam UpdateResultType A template that wraps the type the callback receives
	 * \tparam Sink The type of the object that should receive the new value
	 * \tparam T The type of the source value
	 */
	template<template<class> class UpdateResultType, class Sink, class T>
	using update_func_t = UpdateResultType<T> (*)(Sink&, update_param_t<T>) UPDATE_CALLBACK;

	/**
	 * \brief Type that indicates that a suitable overload for update was found
	 */
	struct update_func_found{};

	/**
	 * \brief Checks whether or not UpdateTraits has a member called update which matches update_func_t
	 * \returns The type that failed the check, or update_func_found a suitable overload was found.
	 */
	template<class UpdateTraits, template<class> class R, class Sink, class... Types>
	struct has_applicable_update
	{
		using result = update_func_found;
	};

	template<class UpdateTraits, template<class> class R, class Sink, class T, class... Types>
	struct has_applicable_update<UpdateTraits, R, Sink, T, Types...>
	{
		static constexpr auto current_value = requires(update_func_t<R, Sink, T>& cb) {
			{ cb = &UpdateTraits::update };
		};

		using result = std::conditional_t<
			current_value,
			typename has_applicable_update<UpdateTraits, R, Sink, Types...>::result,
			T
		>;
	};

	/**
	 * \brief Concept used to check that UpdateTraits satisfies all requirements
	 */
	template<class UpdateTraits, template<class> class UpdateResultType, class Sink, class... Types>
	concept update_traits = std::same_as<
		typename has_applicable_update<UpdateTraits, UpdateResultType, Sink, Types...>::result,
		update_func_found
	>;

	/**
	 * \brief A type erased wrapper around an entity that can store a value
	 *
	 * \tparam UpdateResultType \see update_func_t
	 * \tparam Types the types that the wrapped value_storage should accept
	 */
	template<template<class> class UpdateResultType, class... Types>
	class value_storage
	{
	public:
		constexpr value_storage() = default;

		// update_traits<UpdateResultType, Sink, Types...>

		/**
		 * \brief Initializes the value_storage with target. UpdateTraits must provide
		 *        overloads of the static function update for each type, that accepts a
		 *        as Sink reference, and the specific type. The function should return
		 *        the type wrapped in UpdateResultType, which typically results in a
		 *        pointer or a reference.
		 */
		template<class Sink, class UpdateTraits>
		constexpr explicit value_storage(Sink& target, std::type_identity<UpdateTraits> /*unused*/):
				m_handle{&target},
				m_vtable{&s_vtable<Sink, UpdateTraits>}
		{}

		/**
		 * \brief Updates the value_storage with value
		 */
		template<class SourceValue>
		requires(
			!std::is_lvalue_reference_v<SourceValue> ||
			pass_by_value_v<std::remove_cvref_t<SourceValue>>
		)
		[[gnu::always_inline]] [[nodiscard]] constexpr auto update_with(SourceValue&& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			return std::get<update_callback_t<raw_type>>(*m_vtable)(m_handle, std::forward<SourceValue>(value));
		}

		template<class SourceValue>
		requires(!pass_by_value_v<std::remove_cvref_t<SourceValue>>)
		[[gnu::always_inline]] [[nodiscard]] constexpr auto update_with(SourceValue const& value) const
		{
			using raw_type = std::remove_cvref_t<SourceValue>;
			if constexpr(std::is_trivially_copyable_v<std::remove_cvref_t<SourceValue>>)
			{ return std::get<update_callback_t<raw_type>>(*m_vtable)(m_handle, value); }
			else
			{ return std::get<update_callback_t<raw_type>>(*m_vtable)(m_handle, raw_type{value}); }
		}

		[[gnu::always_inline]] constexpr operator bool() const
		{ return m_handle != nullptr; }

	private:
		template<class T>
		using update_callback_t = UpdateResultType<T> (*)(void*, update_param_t<T>) UPDATE_CALLBACK;

		using vtable = std::tuple<update_callback_t<Types>...>;

		void* m_handle{nullptr};
		vtable const* m_vtable{nullptr};

		template<class Sink, class UpdateTraits>
		static constexpr vtable s_vtable{
			[](void* target, update_param_t<Types> value) UPDATE_CALLBACK -> UpdateResultType<Types> {
				if constexpr(pass_by_value_v<Types> || std::is_trivially_copyable_v<Types>)
				{ return UpdateTraits::update(*static_cast<Sink*>(target), value); }
				else
				{ return UpdateTraits::update(*static_cast<Sink*>(target), std::move(value)); }
			}...
		};
	};
}

#endif
