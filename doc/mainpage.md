# Jopp

## The file format

A Jopp document stores values of different types

* Numeric tuple
* String
* Boolean
* Byte array
* Object
* Generic

as well as a corresponding array type for each of these types.

### Numeric tuple

A numeric tuple stores a fixed number of numeric values of a certain type. Numbers are paresed
according to the rules of `std::from_chars` as specified by ISO 14882. Optionally, a type prefix
can be specified. If no type prefix is given, a single double-precision float is stored. Otherwise,
the type prefix denotes

* The size of the stored numbers
* The number of elements in the tuple
* Whether or not the numbers are floats or integers
* Whether or not the numbers are signed or unsigned

The size is encoded by a category letter

| Category | Number size in bytes | Example C data type |
|----------|----------------------|---------------------|
| a        | 1                    | int8_t              |
| b        | 2                    | int16_t             |
| c        | 4                    | int32_t             |
| d        | 8                    | int64_t             |

The number elements in the tuple uses category letters x-w

| Category | Number of elements in the tuple |
|----------|---------------------------------|
| x        | 1                               |
| y        | 2                               |
| z        | 3                               |
| w        | 4                               |

Float and integers are differentiated by the letters `f` and `i` respectively. Signed/unsigned are
differentiated by the letters `s` and `u`.

Following the scheme above gives that the letter *combination* for the default type is `dxfs`.
Since it is the combination that defines the type, the order does not matter.

Not all combinations are possible. The type encoding scheme is orthogonal for easier extensibility.
A floating point value is always signed, and never one byte. While there exists both unsigned floats
and 8-bit floats, there is, at the time of writing, no standard for these. Therefore, types such as
`axfs`, `axfu`, `bxfu`, `cxfu`, and `dxfu` are unsupported.

### String

A string can either be serialized as specified by ISO 21778, or according to the rule

```ebnf
String = `s(` any byte except NUL `)'
```

Any character except `(` `\` and `)` are treated literally. A `(` will increment a counter. `)`
decrements the same counter. The string ends when the counter reaches zero. If `(` or `)`is preceded
by `\`, this `\` is discarded and the counter is not updated. A `\` that is not followed
by `(` or `)` is treated literally.


## Compoent diagram

\startuml "Jopp components"
package common_traits
package template_param_pack
\enduml



