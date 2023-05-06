//@	{"target":{"name":"joppdemo.o"}}

#include "lib/parser.hpp"
#include "lib/serializer.hpp"
#include <unistd.h>
#include <errno.h>

int main()
{
	jopp::value root;

	{
		// Read data from stdin
		jopp::parser parser{root};
		std::array<char, 4096> buffer{};
		while(true)
		{
			auto const bytes_read = ::read(STDIN_FILENO, std::data(buffer), std::size(buffer));
			if(bytes_read == 0)
			{ break; }

			if(bytes_read == -1 && errno == EAGAIN)
			{
				// In a real scenario, control should be moved to a different procedure, probably
				// selected by select(2) et.al.
				continue;
			}

			auto const res = parser.parse(std::span{std::data(buffer), static_cast<size_t>(bytes_read)});
			if(res.ec == jopp::parser_error_code::completed)
			{
				// Data is now in "root"
				break;
			}
			else
			if(res.ec != jopp::parser_error_code::more_data_needed)
			{
				// Error condition: invalid data
				return - 1;
			}
		}

		if(is_null(root))
		{
			// Failed due to early eof
				return -1;
		}
	}

	{
		// Write data to stdout
		jopp::serializer serializer{root};
		std::array<char, 4096> buffer{};
		while(true)
		{
			auto const res = serializer.serialize(buffer);
			auto write_ptr = std::begin(buffer);
			auto bytes_to_write = res.ptr - std::begin(buffer);
			while(bytes_to_write != 0)
			{
				auto const bytes_written = ::write(STDOUT_FILENO, write_ptr, bytes_to_write);
				if(bytes_written == 0)
				{ return -1; }

				if(bytes_written == -1 && errno == EAGAIN)
				{
					// In a real scenario, control should be moved to a different procedure,
					// probably selected by select(2) et.al.
					continue;
				}

				write_ptr += bytes_written;
				bytes_to_write -= bytes_written;
			}

			if(res.ec == jopp::serializer_error_code::completed)
			{
				// Success
				return 0;
			}
			if(res.ec != jopp::serializer_error_code::buffer_is_full)
			{
				// Error condition: invalid data
				return -1;
			}
		}
	}
}