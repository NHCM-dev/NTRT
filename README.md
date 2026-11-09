# NTRT

(Credit to GPT 6.1)

N Trading Research Terminal is a local C++ program designed to let user trade and analyze the market.
Using Qt as dependencies.

## Build

NTRT requires CMake 3.21+, Ninja and a C/C++ compiler with C++23 support.
The default presets prefer MinGW / MSYS2 UCRT64. Build results will be in directory `build/`.

The build script will use an existing Qt installation, or download and build Qt in the build directory.
Program will dynamically link the Qt library during runtime. Building Qt for the first time may take a while and require additional tools.

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
