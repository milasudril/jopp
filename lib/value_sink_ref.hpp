#ifndef JOPP_VALUE_SINK_REF_HPP
#define JOPP_VALUE_SINK_REF_HPP

#include "./exception.hpp"
#include "lib/template_param_pack.hpp"

#include <cstddef>
#include <array>
#include <bit>
#include <functional>
#include <cstddef>

namespace jopp2
{
	template<size_t N>
	union array_union;

	template<size_t CurrentN, size_t SrcSize>
	constexpr void set(array_union<CurrentN>& u, std::array<char, SrcSize> const& src);

	template<size_t TargetSize, size_t CurrentN>
	constexpr decltype(auto) get(array_union<CurrentN> const& u);

	template<size_t N>
	union array_union
	{
		std::array<char, N> value;
		array_union<N - 1> next{};

		array_union() = default;
		array_union& operator=(array_union const&) = default;
		array_union& operator=(array_union&&) = default;
		array_union(array_union const&) = default;
		array_union(array_union&&) = default;
		~array_union() = default;

		template<size_t M>
		constexpr array_union& operator=(const std::array<char, M>& src) noexcept
		{
			static_assert(M <= N && M > 0, "Source array exceeds union size");
			set(*this, src);
			return *this;
		}

		template<size_t M>
		constexpr array_union(const std::array<char, M>& src) noexcept
		{
			static_assert(M <= N && M > 0, "Source array exceeds union size");
			set(*this, src);
		}
	};

	template<>
	union array_union<1>
	{ std::array<char, 1> value; };

	static_assert(sizeof(array_union<16>) == 16);
	static_assert(std::is_trivially_copyable_v<array_union<16>>);

	template<size_t CurrentN, size_t SrcSize>
	constexpr void set(array_union<CurrentN>& u, std::array<char, SrcSize> const& src)
	{
		static_assert(SrcSize <= CurrentN && SrcSize > 0, "Requested size out of bounds");
		if constexpr (SrcSize == CurrentN)
		{ u.value = src; }
		else
		{ set(u.next, src); }
	}

	template<size_t TargetSize, size_t CurrentN>
	constexpr decltype(auto) get(array_union<CurrentN> const& u)
	{
		static_assert(TargetSize <= CurrentN && TargetSize > 0, "Requested size out of bounds");
		if constexpr (TargetSize == CurrentN)
		{ return u.value; }
		else
		{ return get<TargetSize>(u.next); }
	}

	template<class T>
	concept can_be_bitcasted_to_array = requires(T obj)
	{
		{ std::bit_cast<std::array<char, sizeof(T)>>(obj) };
	};

	template<class SinkTraits>
	class value_sink_ref
	{
	public:
		template<class Sink>
		using source_value_type = SinkTraits::template source_value_type<Sink>;

		template<class Sink>
		using store_value_ret_type = std::invoke_result_t<
			decltype(SinkTraits::store_value),
			Sink&,
			source_value_type<Sink>
		>;

		value_sink_ref() = default;

		template<class Sink>
		constexpr explicit value_sink_ref(Sink& sink) noexcept:
			m_handle{&sink},
			m_current_callback{
				[](void* sink, source_value_type<Sink> source) -> store_value_ret_type<Sink>{
					return SinkTraits::store_value(*static_cast<Sink*>(sink), source);
				}
			}
		{}

		template<class T>
		constexpr decltype(auto) store_value(T&& val) const
		{
			if(m_handle == nullptr)
			{
				SinkTraits::value_sink_is_unset();
				abort();
			}

			using plain_t = std::remove_cvref_t<T>;
			auto const callback = std::get_if<store_value_callback<plain_t>>(&m_current_callback);
			if(callback == nullptr)
			{
				SinkTraits::value_sink_type_mismatch(
					std::type_identity_t<plain_t>{}, m_current_callback.index()
				);
				abort();
			}

			return callback(m_handle, std::forward<T>(val));
		}

	private:
		template<class Sink>
		using store_value_callback = store_value_ret_type<Sink> (*)(
			void*, source_value_type<Sink>
		);

		void* m_handle{nullptr};
		map_template_param_pack_to_type_t<
			std::variant,
			wrap_template_param_pack_elements_t<
				typename SinkTraits::supported_sink_types,
				store_value_callback
			>
		> m_current_callback;
	};
}

#endif
