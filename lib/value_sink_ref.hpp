#ifndef JOPP_VALUE_SINK_REF_HPP
#define JOPP_VALUE_SINK_REF_HPP

#include "./template_param_pack.hpp"
#include "./variant_utils.hpp"

#include <cstddef>
#include <array>
#include <bit>
#include <functional>

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

	template<class T>
	struct source_value_type_tag
	{ using type = T; };

	template<class T>
	using source_value_type_tag_t = source_value_type_tag<T>::type;

	template<class T>
	struct sink_type_tag
	{ using type = T; };

	struct value_sink_ref_unset_tag
	{
		using sink_type = value_sink_ref_unset_tag;
	};

	template<class T>
	using sink_type_tag_t = sink_type_tag<T>::type;

	template<class SinkTraits>
	class value_sink_ref
	{
	public:
		template<class Sink>
		class sink_wrapper
		{
		public:
			using sink_type = Sink;

			explicit sink_wrapper(Sink& sink):
				m_sink{sink}
			{}
			template<class T>
			requires requires(T&& source_val, Sink& s){
				{SinkTraits::store_value(s, std::forward<T>(source_val))};
			}
			decltype(auto) store_value(T&& source_val) const
			{ return SinkTraits::store_value(m_sink.get(), std::forward<T>(source_val)); }

		private:
			std::reference_wrapper<Sink> m_sink;
		};

		value_sink_ref() = default;

		template<class Sink>
		constexpr explicit value_sink_ref(Sink& sink) noexcept:
			m_sink{sink_wrapper{sink}}
		{}

		template<class T>
		constexpr decltype(auto) store_value(T&& val) const
		{
			return visit_with_args(
				m_sink,
				[]<class SinkWrapper, class SourceValue>(SinkWrapper sink, SourceValue&& source_val) -> std::remove_cvref_t<T>& {
					using sink_type = SinkWrapper::sink_type;
					if constexpr(
						requires{
							{sink.store_value(std::forward<SourceValue>(source_val))}
								->std::same_as<std::remove_cvref_t<T>&>;
						}
					)
					{ return sink.store_value(std::forward<SourceValue>(source_val)); }
					else
					{
						SinkTraits::value_sink_type_mismatch(
							source_value_type_tag<std::remove_cvref_t<SourceValue>>{},
							sink_type_tag<sink_type>{}
						);
						abort();
					}
				},
				std::forward<T>(val)
			);
		}

	private:
		map_template_param_pack_to_type_t<
			std::variant,
			append_to_template_param_pack_t<
				wrap_template_param_pack_elements_t<
					typename SinkTraits::supported_sink_types,
					sink_wrapper
				>,
				value_sink_ref_unset_tag
			>
		> m_sink{value_sink_ref_unset_tag{}};
	};
}

#endif
