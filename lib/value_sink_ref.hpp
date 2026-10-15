#ifndef JOPP_VALUE_SINK_REF_HPP
#define JOPP_VALUE_SINK_REF_HPP

#include "./template_param_pack.hpp"
#include "./variant_utils.hpp"

#include <functional>

namespace jopp2
{
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

		constexpr bool operator==(value_sink_ref_unset_tag /*unused*/) const
		{ return true; }

		constexpr bool operator!=(value_sink_ref_unset_tag /*unused*/) const
		{ return false; }
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

			constexpr explicit sink_wrapper(Sink& sink):
				m_sink{sink}
			{}

			template<class T>
			requires requires(T&& source_val, Sink& s){
				{SinkTraits::store_value(s, std::forward<T>(source_val))};
			}
			constexpr decltype(auto) store_value(T&& source_val) const
			{ return SinkTraits::store_value(m_sink.get(), std::forward<T>(source_val)); }

			constexpr auto& get() const
			{ return m_sink.get(); }

			constexpr bool operator==(sink_wrapper other) const
			{ return &m_sink.get() == &other.get(); }

			constexpr bool operator!=(sink_wrapper other) const
			{ return !(*this == other); }

		private:
			std::reference_wrapper<Sink> m_sink;
		};

		value_sink_ref() = default;

		template<class Sink>
		constexpr explicit value_sink_ref(Sink& sink) noexcept:
			m_sink{sink_wrapper{sink}}
		{}

		struct do_store_value
		{
			template<class SinkWrapper, class SourceValue>
			requires requires(SinkWrapper sink, SourceValue&& source_val){
				{sink.store_value(std::forward<SourceValue>(source_val))};
			}
			static constexpr decltype(auto) operator()(SinkWrapper sink, SourceValue&& source_val)
			{
				return sink.store_value(std::forward<SourceValue>(source_val));
			}
		};

		template<class T>
		constexpr decltype(auto) store_value(T&& val) const
		{
#if 0
			static_assert(std::is_invocable_v<do_store_value, sink_wrapper<int>, int>);

			using ret_type = return_type_of_first_supported_type_t<
				storage, do_store_value, T
			>;
#endif

			return visit_with_fallback_action_and_args(
				m_sink,
				[]<class SinkWrapper, class SourceValue>(
					SinkWrapper sink,
					SourceValue&& source_val
				) -> decltype(auto) requires requires(SinkWrapper sink, SourceValue&& source_val){
				{sink.store_value(std::forward<SourceValue>(source_val))};
			}{
					return sink.store_value(std::forward<SourceValue>(source_val));
				},
				[]<class SinkWrapper, class SourceValue>(
					SinkWrapper /*unused*/,
					SourceValue&& /*unused*/
				) {
						SinkTraits::value_sink_type_mismatch(
							source_value_type_tag<std::remove_cvref_t<SourceValue>>{},
							sink_type_tag<typename SinkWrapper::sink_type>{}
						);
						abort();
				},
				std::forward<T>(val)
			);

#if 0
	constexpr decltype(auto) visit_with_fallback_action_and_args(
		VariantType&& to_visit,
		Visitor&& visitor,
		FallbackAction&& fallback,
		VisitorArgs&&... args
	)

			printf("%s\n", typeid(ret_type).name());

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
					{ return do_store_value::operator()(sink, std::forward<SourceValue>(source_val)); }
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
#endif
		}

		template<class T>
		constexpr bool is_bound_to(T& other) const
		{
			auto const ptr = std::get_if<sink_wrapper<std::remove_cvref_t<T>>>(&m_sink);
			return ptr != nullptr && &other == &ptr->get();
		}

		constexpr bool is_bound_to_same_as(value_sink_ref other) const
		{ return m_sink == other.m_sink; }

		constexpr operator bool() const
		{ return std::get_if<value_sink_ref_unset_tag>(&m_sink) == nullptr; }

	private:
		using storage = map_template_param_pack_to_type_t<
			std::variant,
			append_to_template_param_pack_t<
				wrap_template_param_pack_elements_t<
					typename SinkTraits::supported_sink_types,
					sink_wrapper
				>,
				value_sink_ref_unset_tag
			>
		>;
		storage m_sink{value_sink_ref_unset_tag{}};
	};
}

#endif
