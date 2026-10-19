# Jopp

## Syntax

The following is a summary of the jopp syntax

```ebnf
message = [value_type_id], value ;
value_type_id = (number_type_id , [array_begin_marker])
	| hash_type_id
	| string_begin_tag
	| object_begin_tag ;

number_type_id = { signedness_flag | floatness_flag | size_category | tuple_size_category } ;
signedness_flag = "s" | "u" ;
floatness_flag = "f" | "i" ;
size_category = "a" | "b" | "c" | "d" ;
tuple_size_category = "x" | "y" | "z" | "w" ;

object_begin_tag = [subtype_begin_id_marker, subtype_id_name], object_begin_marker
subtype_id_begin_marker = "!" ;
subtype_id_name = { digit | ascii_letter | "_" | "-" | "." | ":" } ;
object_begin_marker = "{" ;

string_begin_tag = {"str(" | '"'} ;

hash_type_id = "#", size_category, {array_begin_marker | "x"} ;

array_begin_marker = "[" ;
```

### Size categories

| Category | Number of bytes (number) | Number of bytes (hash) |
|----------|--------------------------|------------------------|
| a        | 1                        | 4                      |
| b        | 2                        | 8                      |
| c        | 4                        | 16                     |
| d        | 8                        | 32                     |


## Compoent diagram

\startuml "Jopp components"
package common_traits
package template_param_pack
\enduml



