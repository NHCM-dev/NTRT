# NTRT

N Trading Research Terminal is a C++23 / Qt desktop application for market analysis and trading.

## Build

Requires CMake 3.21+, Ninja, and a compatible C++23 compiler. CMake locates Qt or downloads a compatible SDK; the initial download may require Python.

```shell
cmake --preset release
cmake --build --preset release
```

The Windows package is generated in `build/release/dist/NTRT`. Release builds intentionally omit compiler optimization flags.

## Tests

```shell
cmake --preset release -DNTRT_BUILD_TESTS=ON
cmake --build --preset release
ctest --test-dir build/release --output-on-failure
```
