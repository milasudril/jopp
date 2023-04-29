//@	{"target":{"name":"serializer.test"}}

#include "./serializer.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_serializer_serialzie)
{
	jopp::object obj;

	obj.insert("1. an array", jopp::array{});
	obj.insert("2. an object", jopp::object{});
	obj.insert("3. a number", 1.25);
	obj.insert("4. null literal", jopp::null{});
	obj.insert("5. a bool", true);
	obj.insert("6. a string", "Hello, World");
	
	jopp::value val{std::move(obj)};
	
	jopp::serializer serializer{val};
	
	std::array<char, 1024> buffer{};
	
	serializer.serialize(buffer);
	
	printf("%s\n", buffer.data());;
}
