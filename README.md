# Jopp 2.0

## TODO:s for next release

* Use a template parameter to specify the map implementation

* Add support for decoding surrogate pairs in strings

* Add support for any value at the top level

* Add generic tree visitation algorithm

* Add extension that makes the parser discard the current top-level object. ASCII end-of-text character in the stream will trigger this code path. Use-case: A fault appears in the producer, and it wants to tell the consumer to start over, to parse an error message instead.

* Add extension that makes the parser flush the current top-level object. ASCII form-feed character in the stream will trigger this code path. Use-case: Faster record termination