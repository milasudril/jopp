#ifndef JOPP_WITH_SUBTYPE_ID_HPP
#define JOPP_WITH_SUBTYPE_ID_HPP

#include <utility>

namespace jopp2
{
	template<class SubtypeIdType, class Base>
	class with_subtype_id:public Base
	{
	public:
		using subtype_id_type = SubtypeIdType;

		using Base::Base;

		template<class ... Args>
		constexpr explicit with_subtype_id(subtype_id_type&& subtype_id, Args&&... args):
			Base{std::forward<Args>(args)...},
			m_subtype_id{std::move(subtype_id)}
		{}

		constexpr subtype_id_type const& subtype_id() const
		{ return m_subtype_id;}

	private:
		subtype_id_type m_subtype_id;
	};
}

#endif
