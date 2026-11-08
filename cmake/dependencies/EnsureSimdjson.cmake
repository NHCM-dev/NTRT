include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Pin the release and keep downloads/build artifacts inside the build tree.
set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(simdjson
    URL https://github.com/simdjson/simdjson/archive/refs/tags/v5.0.4.tar.gz
)
function(ntrt_add_simdjson)
    set(BUILD_SHARED_LIBS OFF)
    FetchContent_MakeAvailable(simdjson)
endfunction()
ntrt_add_simdjson()
