# Simple-JSON-Parser (SJP)
`SJP` is a minimal JSON parser that (almost) fully implements the JSON spec at
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
Take a look at `main.cc` for an example of how to use `SJP`. A
[Makefile](./src/Makefile) is provided (again, my setup with `gcc` as the
compiler). Run the following in the repository's base directory:

```bash
make test
```

# To-Do's
The only thing from the spec that we don't currently support are unicode code
points in string literals, i.e. right now `\uXXXX` is just ignored. For all my
purposes, I've never needed this.

There are a few things in the source that might need a fix. They're annotated
with `@TODO` and you can grep for them. Nothing big, though.

Lastly, we don't have a proper test suite right now. I've used this code as a
library quite a bit, but if you find a bug, I'll add regression tests ---
I promise!

# License
All code in this repository is licensed under the [GPLv3](./LICENSE.md).
