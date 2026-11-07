# NTRT

N Trading Research Terminal is a local C++ program designed to let user trade and analyze the market.
Using Qt as dependencies.

## Build

NTRT requires Qt6 and CMake. By running the building command, script will automatically download Qt and put them 
in the build directory. Program will dynamic link the Qt library during runtime.  

Clone the repository:

```shell
git clone https://github.com/NHCM-dev/NTRT.git ./NTRT
```

Build using CMake:

```shell
cd ./NTRT
cmake --preset windows-msys2-release
cmake --build --preset release
```

The distribution files will be available at directory `./build/release/dist`, executable file will be 
at `./build/release/dist/NTRT.exe`


