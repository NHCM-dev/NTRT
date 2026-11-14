include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(ADS_VERSION "4.4.0")
set(BUILD_EXAMPLES OFF CACHE BOOL "Build ADS examples" FORCE)
set(BUILD_STATIC OFF CACHE BOOL "Build ADS shared library" FORCE)
FetchContent_Declare(qtads
    URL https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/archive/refs/tags/4.4.0.tar.gz
    URL_HASH SHA256=ccd0aa64e5bcfdb58b7ad4dda06b656b90571cfc94cb556dafac4b283ac1bb55)
FetchContent_MakeAvailable(qtads)
