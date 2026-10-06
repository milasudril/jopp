#ifndef JOPP_VALUE_SINK_REF_HPP
#define JOPP_VALUE_SINK_REF_HPP

#include "./exception.hpp"

#include <cstddef>
#include <array>
#include <bit>
#include <functional>

namespace jopp2
{
	template<size_t N>
	union array_union;

	template<size_t TargetSize, size_t CurrentN>
	constexpr decltype(auto) get(array_union<CurrentN>& u);

	template<size_t TargetSize, size_t CurrentN>
	constexpr decltype(auto) get(array_union<CurrentN> const& u);

	template<size_t N>
	union array_union
	{
		std::array<char, N> value;
		array_union<N - 1> next;

		array_union& operator=(array_union const&) = default;
		array_union& operator=(array_union&&) = default;
		array_union(array_union const&) = default;
		array_union(array_union&&) = default;
		~array_union() = default;

		template<size_t M>
		constexpr array_union& operator=(const std::array<char, M>& src) noexcept
		{
			static_assert(M <= N && M > 0, "Source array exceeds union size");
			get<M>(*this) = src;
			return *this;
    }

		template<size_t M>
		constexpr array_union(const std::array<char, M>& src) noexcept
		{
			static_assert(M <= N && M > 0, "Source array exceeds union size");
			get<M>(*this) = src;
			return *this;
    }
	};

	template<>
	union array_union<1>
	{ std::array<char, 1> value; };

	static_assert(sizeof(array_union<16>) == 16);
	static_assert(std::is_trivially_copyable_v<array_union<16>>);

	template<size_t TargetSize, size_t CurrentN>
	constexpr decltype(auto) get(array_union<CurrentN>& u)
	{
		static_assert(TargetSize <= CurrentN && TargetSize > 0, "Requested size out of bounds");
		if constexpr (TargetSize == CurrentN)
		{ return u.value; }
		else
		{ return get<TargetSize>(u.next); }
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
		value_sink_ref() = default;

		template<class Sink>
		constexpr explicit value_sink_ref(std::reference_wrapper<Sink> sink) noexcept:
			m_handle{&sink.get()},
			m_store_value{
				[](void* sink, value_to_store source){
					using source_value_type = SinkTraits::template source_value_type<Sink>;
					auto& sink_ref = *static_cast<Sink*>(sink);
					if constexpr(pass_by_value<source_value_type>)
					{
						return value_sink_ref{
							std::ref(
								SinkTraits::store_value(
									sink_ref,
									std::bit_cast<source_value_type>(get<sizeof(source_value_type)>(source.value))
								)
							)
						};
					}
					else
					{
						return value_sink_ref{
							std::ref(
								SinkTraits::store_value(
									sink_ref,
									static_cast<source_value_type const&>(source.ptr)
								)
							)
						};
					}
				}
			},
			m_type_id{SinkTraits::template source_type_id<Sink>}
		{}

		template<class T>
		constexpr value_sink_ref store_value(T&& val) const noexcept
		{
			if(m_handle == nullptr)
			{ raise_internal_error("Unset value_sink_ref"); }

			using plain_t = std::remove_cvref_t<T>;
			if(SinkTraits::template type_id<plain_t> != m_type_id)
			{ raise_internal_error("Type mismatch during assignment"); }

			if constexpr(pass_by_value<plain_t>)
			{
				return m_store_value(
					m_handle,
					value_to_store{
						.value = std::bit_cast<std::array<char, sizeof(plain_t)>>(std::forward<T>(val))
					}
				);
			}
			else
			{ return m_store_value(m_handle, value_to_store{.ptr = &val}); }
		}

		[[nodiscard]] constexpr bool valid() const noexcept
		{ return m_handle != nullptr; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept
		{ return valid(); }

		template<class Sink>
		[[nodiscard]] constexpr bool is_bound_to(Sink const& sink) const noexcept
		{ return m_handle == &sink; }

		template<class T>
		[[nodiscard]] constexpr bool accepts_type() const noexcept
		{
			using plain_t = std::remove_cvref_t<T>;
			return m_type_id == SinkTraits::template type_id<plain_t>;
		}


	private:
		static constexpr auto max_inline_size = 2*sizeof(void*);

		template<class T>
		static constexpr bool pass_by_value = can_be_bitcasted_to_array<T>
			&& sizeof(T) <= max_inline_size;

		union value_to_store
		{
			void const* ptr;
			array_union<max_inline_size> value;
		};

		void* m_handle{nullptr};
		value_sink_ref (*m_store_value)(void* sink, value_to_store source) = nullptr;
		size_t m_type_id = static_cast<size_t>(-1);
	};
}

#endif
