# Jopp

Jopp is another JSON library for C++. Jopp

* Is header-only to simplify integration. The parser can read from different input ranges.

* Supports non-blocking I/O

* Does not complain when there is more data to be processed after the first JSON object/array has
ended

* Parses/serializes numbers with `std::from_chars`/`std::to_chars`. This means that inf and nan are
supported. Please notice that the behaviour with regards to inf and nan may depend on compiler
options such as `-ffinite-math`.

* Always maps a JSON `number` to the type double. This differs from the choice made by jansson and
nlohmann, which may parse numbers to integers. This choice has been made since there is no standard
way of inferring that a number written as an integer should actually be a double. Also, this choice
prevents information loss, when the data should be pared by other implementations.

* Has an optional limit on the tree depth to control memory usage. By default, it is set
to 1024 levels.

* Since the parser works on blocks of data, it is possible to stop feeding data at a certain point.
This is also a measure to prevent control memory usage.

* Objects uses `std::map` as backing store. This means that Jopp is immune against hash attacks.
Also data serialized by Jopp is always consistent in formatting, which helps when comparing files.

## Example usage

The following program demonstrates how to read data from stdin, and write it back to stdout. For details about different features, see the corresponding file:

| Feature                             | Include file       |
| ----------------------------------- | ------------------ |
| Delimiters and escape char handling | lib/delimiters.hpp |
| Misc                                | lib/utils.hpp      |
| Parser                              | lib/parser.hpp     |
| Serializer                          | lib/serializer.hpp |
| Data storage                        | lib/types.hpp      |

```c++
#include <jopp/parser.hpp>
#include <jopp/serializer.hpp>
#include <unistd.h>
#include <errno.h>

int main()
{
	jopp::container root;

	{
		// Read data from stdin
		jopp::parser parser{root};
		std::array<char, 4096> buffer{};
		auto old_ec = jopp::parser_error_code::completed;
		while(true)
		{
			auto const bytes_read = ::read(STDIN_FILENO, std::data(buffer), std::size(buffer));
			if(bytes_read == 0)
			{
				if(old_ec != jopp::parser_error_code::completed)
				{
					fprintf(stderr, "error: %s\n", to_string(old_ec));
					return -1;
				}
				break;
			}

			if(bytes_read == -1 && errno == EAGAIN)
			{
				// In a real scenario, control should be moved to a different procedure, probably
				// selected by select(2) et.al.
				continue;
			}

			auto const res = parser.parse(std::span{std::data(buffer), static_cast<size_t>(bytes_read)});
			if(res.ec == jopp::parser_error_code::completed)
			{ break; }
			else
			if(res.ec != jopp::parser_error_code::more_data_needed)
			{
				fprintf(stderr, "error: %s\n", to_string(res.ec));
				// Error condition: invalid data
				return - 1;
			}
			old_ec = res.ec;
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
```
