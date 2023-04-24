# Jopp

Jopp is another JSON parser for C++. It features a DOM parser, that

* Allows streams that does not end with the outermost json object
* Can be used together with non-blocking I/O

## Example usage

```c++
#include <jopp/parser.hpp>
#include <unistd.h>
#include <errno.h>

int main()
{
	jopp::parser parser;
	jopp::value root;
	std::array<char, 4096> buffer{};
	while(true)
	{
		auto const bytes_read = ::read(STDIN_FILENO, std::data(buffer), std::size(buffer));
		if(bytes_read == 0)
		{ break; }

		if(bytes_read == -1 && errno == EAGAIN)
		{ continue; }

		auto const res = parser.parse(std::span{std::data(buffer), bytes_read}, root);
		if(res.ec == jopp::error_code::completed)
		{
			// Data is now in "root"
			break;
		}
		else
		if(res.ec != jopp::error_code::more_data_needed)
		{
			// Error condition: input contains invalid data
		}
	}

	if(is_null(root))
	{
		// Failed due to early eof
	}
}
```
