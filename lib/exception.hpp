#ifndef JOPP_EXCEPTION_HPP
#define JOPP_EXCEPTION_HPP

#include <stdexcept>
#include <format>
#include <print>
#include <source_location>

namespace jopp2
{
	/**
	 * \brief Class used for exception raised by jopp
	 */
	class exception:public std::exception
	{
	public:
		template< class... Args >
		constexpr explicit exception(std::format_string<Args...> fmt, Args&&... args ):
			m_message{std::format(fmt, std::forward<Args>(args)...)}
		{}

		constexpr char const* what() const noexcept override
		{ return m_message.c_str(); }

	private:
		std::string m_message;
	};

	template<class... Args>
	[[noreturn]] [[gnu::cold]] void raise_internal_error(
		std::format_string<std::remove_cvref_t<Args> const&...> fmt,
		std::tuple<Args...> const& args = std::tuple{},
		std::source_location loc = std::source_location::current()
	)
	{
		static_assert(std::is_trivially_copyable_v<std::format_string<std::remove_cvref_t<Args> const&...>>);
		static_assert(sizeof(std::format_string<std::remove_cvref_t<Args> const&...>) == 2*sizeof(void*));

		auto msg = std::apply(
			[fmt](std::remove_cvref_t<Args> const&... args) {
					return std::format(fmt, args...);
			},
			args
		);

		std::print(
			stderr,
			"jopp internal error: {}:{}: {}\n",
			loc.file_name(),
			loc.line(),
			std::move(msg)
		);
		fflush(stderr);
		abort();
	}
}

#endif