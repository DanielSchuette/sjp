<p align="center">
    <a href="https://github.com/DanielSchuette/sjp" alt="Contributors">
        <img src="https://img.shields.io/badge/version-0.1-blue" /></a>
</p>

# Simple-JSON-Parser (sjp)
`sjp` is a minimal JSON parser that (almost) fully implements the JSON spec at
[www.json.org](https://www.json.org/json-en.html). It is written in fairly
clean C++ and the interface is designed to make data from a JSON file easily
accessible without a complicated API or any extra dependencies. The following
headers are used internally and most of them could be replaced to be even more
minimal:

- `cassert`
- `cmath`
- `cstdarg`
- `cstdint`
- `cstdio`
- `cstdlib`
- `cstring`
- `algorithm`
- `unordered_map`
- `string`
- `queue`

The small logger I usually use depends on the following headers:

- `ctime`
- `unistd.h`

The logger interface is extremely minimal, too. If you're not on Linux and/or
want to use your own logger, just implement:

```c++
class YourLogger : public io::Logger {
public:
    virtual void log(const char* fmt, ...)  const override { /* ... */ }
    virtual void warn(const char* fmt, ...) const override { /* ... */ }
    [[noreturn]] virtual void log(const char* fmt, ...) const override
    { /* ... */ }
};
```

Then, replace the contents of `io.cc` with your implementation.

The `io.hh` header actually provides a `NullLogger` that doesn't do anything
and can be used if no logging should be done. Without a logger, you will only
know about parsing errors when you cannot access a specific property on your
JSON object.

We _do_ use `new`/`delete` for heap allocations and do not specify an interface
for using a custom allocator. Shouldn't be too hard to do, if that's a future
requirement. You can verify the fact that we don't leak any allocations with
`make leak-test` (requires `valgrind`).

# Usage Example
Take a look at [`src/main.cc`](./src/main.cc) for an example of how to use
`sjp`. A [Makefile](./src/Makefile) is provided (again, my setup with `gcc` as
the compiler). Run `make test` to see what `sjp` can do for you.

Here's a snippet, taken from that file:

```c++
{
    FILE* stream = fopen("some/file.json", "r");
    auto logger = io::SjpLogger(*argv, stderr);
    auto parser = sjp::Parser(stream, &logger);

    sjp::Json json = parser.parse();
    json.print(stderr); // pretty-prints the parsed JSON to a FILE*

    /* Now, we can read data from the SJP::JSON object.
     * JSONOBJECTs are accessed via OPERATOR[] and string keys.
     */
    sjp::JsonValue& array = json["data"]["deeply"]["nested"];
    assert(array.get_type() == sjp::Type::Array);

    std::vector<double> v;
    for (size_t i = 0; i < array.size(); i++) {
        sjp::JsonValue& item = array[i];

        /* JSONARRAYs are accessed via OPERATOR[] and integer keys.
         * We get a JSONVALUE&, which we must cast to the actual type before
         * the VALUE member (that every primitive type has) can be accessed.
         * As seen above, we can always check JSONVALUE.GET_TYPE() to
         * dynamically validate what kind of data we've got.
         */
        if (item.get_type() == sjp::Type::Number) {
            sjp::JsonNumber& n = static_cast<sjp::JsonNumber&>(item);
            v.push_back(n.value);
        }

        /* The (probably better) alternative is to use polymorphic data
         * accessors that return STD::OPTIONAL-wrapped values. Those can then
         * be checked for actual content using the familiar C++ STL functions:
         */
        std::optional<double> opt_num = item.get_number();
        if (opt_num) v.push_back(*opt_num);
    }
}
```

`sjp` only has a few API functions you need to know about and those are pretty
much all demonstrated in [`src/main.cc`](./src/main.cc).

Since we don't link any external dependencies in, any IDE should be easily able
to compile the code, too. When you use `sjp` as a library, simply copy `src/*`
into your project and include `sjp.hh` and `io.hh` where you need the parser
and/or the `JSON` object. Yes, it is honestly __that__ easy!

# To-Do's
The only thing from the spec that we don't currently support are unicode code
points in string literals, i.e. right now `\uXXXX` is just ignored. For all my
purposes, I've never needed this.

There are a few things in the source that might need a fix. They're annotated
with `@TODO` and you can grep for them. Nothing big, though.

Lastly, we don't have a proper test suite right now. I've used this code as a
library quite a bit, but if you find a bug, I'll add regression tests -
I promise!

Maybe we should prefix logger callers with the library name to improve the
readability of logging outputs. I usually prefer to once log the filename of
the JSON file that I'm parsing before I start the parser.

# License
All code in this repository is licensed under the [GPLv3](./LICENSE.md).
