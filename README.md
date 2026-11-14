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

## Market data

Market has a resizable instrument list and a reserved chart pane. New settings default to the dark theme with Type and Day Volume hidden; existing saved preferences are preserved.

`DataMonitor` owns ticker feeds, `CandlesManager`, and instrument icons. Daily change uses `(latest ticker price - daily open) / daily open`, and volume uses the daily candle's base and quote currency fields. Settings ¡ú Trading ¡ú Trading data time zone selects midnight UTC (default, OKX `1Dutc`) or UTC+8 (OKX `1D`). Changing the time zone reloads candle history without reusing the previous zone's candles. The choice is saved in `market.json`; older configurations default to UTC. The candle manager keeps up to 300 daily bars per instrument in memory, refreshes about every 10 seconds, and retries failures after 30 seconds while retaining cached data. Icons come from LogoAPI.sh with a letter fallback and a disk cache under `app/cache/instrument-icons`.

To run candle parsing, rollover, subscription lifecycle, cache and settings checks:

```shell
cmake --preset debug -DNTRT_BUILD_TESTS=ON
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
```
