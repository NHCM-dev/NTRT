# Prefer MSYS2 UCRT64; allow other MinGW installations on PATH.
find_program(_ntrt_gcc NAMES gcc HINTS "$ENV{MSYS2_ROOT}/ucrt64/bin" "C:/msys64/ucrt64/bin" REQUIRED)
get_filename_component(_ntrt_bin "${_ntrt_gcc}" DIRECTORY)
find_program(_ntrt_gxx NAMES g++ HINTS "${_ntrt_bin}" NO_DEFAULT_PATH REQUIRED)
set(CMAKE_C_COMPILER "${_ntrt_gcc}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER "${_ntrt_gxx}" CACHE FILEPATH "" FORCE)
get_filename_component(_ntrt_prefix "${_ntrt_bin}/.." ABSOLUTE)
list(PREPEND CMAKE_PREFIX_PATH "${_ntrt_prefix}")
set(ENV{PATH} "${_ntrt_bin};$ENV{PATH}")
