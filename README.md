# C++ utilities
Miscellaneous C++ utilities accumulated over the years. 

## Components
### make_ptr.h
`make_ptr<object>(...)` function to create an appropriate shared pointer based on the object's `ptr` type (or `ptr_templ` template). Correctly dispatches to make_shared if needed.
```cpp
struct object {
    template <typename T>
    using ptr_templ = std::shared_ptr<T>;
};

struct derived: public object {
    derived(int x) {...}
};
...

auto ptr = make_ptr<derived>(12);

```

### ptr.h
Intrusive reference counting pointer with support for weak pointers

### enum_util.h: 
Rather trivial boilerplate code to use `enum class` as bitmap. Use `ENABLE_BITMAP_OPERATORS(enum)` in global scope to enable.

### mach_clock.{h,cpp}
std::chrono-style wrapper for `mach_absolute_time()`

## Building and running the tests.

This project uses CMake to build the unit tests. Otherwise, it's probably easier to just copy the files where needed instead of packaging as a library.

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
ctest .
```
You'll need gtest installed somewhere to build the tests.


## License
See [LICENSE.txt](LICENSE.txt) for details.

