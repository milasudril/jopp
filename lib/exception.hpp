#ifndef JOPP_EXCEPTION_HPP
#define JOPP_EXCEPTION_HPP

#include <format>
#include <print>
#include <source_location>

#ifdef COVERAGE_BUILD
extern "C"
{
	void __gcov_dump();
}
#endif

namespace jopp2
{
#ifdef COVERAGE_BUILD
	inline void flush_errstream(FILE* stream)
	{
		fflush(stream);
		__gcov_dump();
	}
#else
	inline void flush_errstream(FILE* stream)
	{ fflush(stream); }
#endif

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

	/**
	 * \brief Function called when an unrecoverable error is detected within jopp
	 */
	template<class... Args>
	[[noreturn]] [[gnu::cold]] void raise_internal_error(
		std::format_string<std::remove_cvref_t<Args> const&...> fmt,
		std::tuple<Args...> const& args = std::tuple{},
		std::source_location loc = std::source_location::current()
	)
	{
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
		flush_errstream(stderr);
		abort();
	}

	/**
	 * \brief Helper function to pack args into a tuple so they can be passed to raise_internal_error
	 */
	template<class... Args>
	inline constexpr std::tuple<Args...> make_fmt_args(Args&&... args)
	{ return std::tuple<Args&&...>(std::forward<Args>(args)...); }
}

#endif
