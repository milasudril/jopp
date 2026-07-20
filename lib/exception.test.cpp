//@	{"target":{"name":"exception.test"}}

#include "./exception.hpp"

#include <testfwk/testfwk.hpp>
#include <unistd.h>
#include <sys/wait.h>

TESTCASE(jopp2_exception_what)
{
	jopp2::exception foo{"This is a test {}", 123};
	EXPECT_EQ(foo.what(), std::string_view{"This is a test 123"});
	EXPECT_EQ((std::is_base_of_v<std::exception, jopp2::exception>), true);
}

namespace
{
	struct file_deleter
	{
		static void operator()(FILE* file)
		{
			if(file != nullptr)
			{ fclose(file); }
		}
	};

	std::string read_until_closed(int fd)
	{
		std::unique_ptr<FILE, file_deleter> file{fdopen(fd, "rb")};
		std::string ret;
		while(true)
		{
			auto const ch_in = getc(file.get());
			if(ch_in == EOF)
			{ return ret; }
			ret += static_cast<char>(ch_in);
		}
		return ret;
	}
}

TESTCASE(jopp2_raise_internal_error)
{
	std::array<int, 2> errpipe{};
	auto const res = pipe(std::data(errpipe));
	if(res == -1)
	{ jopp2::raise_internal_error("Syscall pipe failed"); }

	auto const child = fork();
	if(child == -1)
	{ jopp2::raise_internal_error("Syscall fork failed"); }


	if(child == 0)
	{
		dup2(errpipe[1], STDERR_FILENO);
		close(errpipe[0]);
		jopp2::raise_internal_error("Something went wrong {} {}", std::tuple{1, "Foobar"});
		_exit(-1);
	}
	else
	{
		close(errpipe[1]);
		auto const output = read_until_closed(errpipe[0]);
		int w_status{};
		auto const res = waitpid(child, &w_status, 0);
		EXPECT_EQ(res, child);
		EXPECT_EQ(WIFEXITED(w_status), false);
		EXPECT_EQ(WIFSIGNALED(w_status), true);
		EXPECT_EQ(WTERMSIG(w_status), SIGABRT);
		EXPECT_EQ(output, "jopp internal error: lib/exception.test.cpp:58: Something went wrong 1 Foobar\n");
	}
}