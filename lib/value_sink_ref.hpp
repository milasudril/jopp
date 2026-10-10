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
