//@	{"target":{"name":"serializer.test"}}

#include "./serializer.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(jopp_serializer_serialzie)
{
	jopp::array array;
	array.push_back(0.5);
	array.push_back("Some text");
	array.push_back(false);
	
	jopp::object subobj;
	subobj.insert("Key", "Value");
	
	jopp::object obj;
	obj.insert("1. an array", std::move(array));
	obj.insert("2. an object", std::move(subobj));
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
