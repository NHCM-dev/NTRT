# NTRT

(Credit to GPT 6.1)

N Trading Research Terminal is a local C++ program designed to let user trade and analyze the market.
Using Qt as dependencies.

## Build

NTRT requires CMake 3.21+, Ninja and a C/C++ compiler with C++23 support.
The default presets prefer Clang (MSYS2 CLANG64 or clang-cl), then fall back to MinGW GCC or MSVC. They honor an explicitly selected compiler. Build results are under `build/`.

The build uses a compatible Qt SDK or downloads a prebuilt one under `build/`. MSYS2 MinGW and CLANG64 use their matching Qt packages. Clang-cl uses the MSVC Qt SDK. MSVC and supported standalone MinGW builds use the official Qt SDK and need Python for the first download.

Clone the repository:

```shell
git clone https://github.com/NHCM-dev/NTRT.git ./NTRT
cd ./NTRT
```

Build using CMake:

```shell
cmake --preset release
cmake --build --preset release
```

Distribution files will be available at `./build/release/dist/NTRT`, including the executable,
required DLLs and resources. This directory is cleared when the Release program is rebuilt.

To explicitly select Visual Studio 2022, use `msvc-release` in both commands. Its output is under `build/msvc-release`. Use a fresh build directory when changing compilers.
