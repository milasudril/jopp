#ifndef JOPP_EXCEPTION_HPP
#define JOPP_EXCEPTION_HPP

#include <stdexcept>
#include <format>

namespace jopp2
{
	/**
	 * \brief Class used for exception raised by jopp
	 */
	class exception:public std::exception
	{
	public:
		template< class... Args >
		explicit exception(std::format_string<Args...> fmt, Args&&... args ):
			m_message{std::format(fmt, std::forward<Args>(args)...)}
		{}

		char const* what() const noexcept override
		{ return m_message.c_str(); }

	private:
		std::string m_message;
	};
}

#endif