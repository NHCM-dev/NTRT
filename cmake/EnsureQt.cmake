option(NTRT_AUTO_INSTALL_QT "Build a local shared Qt if no installed Qt is found" ON)
set(NTRT_QT_VERSION "6.8.3" CACHE STRING "Fallback Qt source version")
set(_local_qt "${CMAKE_BINARY_DIR}/_deps/qt-install")
list(PREPEND CMAKE_PREFIX_PATH "${_local_qt}")
find_package(Qt6 6.5 QUIET COMPONENTS Widgets)
if(NOT Qt6_FOUND)
    if(NOT NTRT_AUTO_INSTALL_QT)
        message(FATAL_ERROR "Qt not found; set CMAKE_PREFIX_PATH or enable NTRT_AUTO_INSTALL_QT")
    endif()
    message(STATUS "Qt missing: downloading and building shared Qt in ${CMAKE_BINARY_DIR}/_deps (first configure may take a long time)")
    include(FetchContent)
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" _qt_series "${NTRT_QT_VERSION}")
    FetchContent_Declare(ntrt_qt_source
        URL "https://download.qt.io/archive/qt/${_qt_series}/${NTRT_QT_VERSION}/submodules/qtbase-everywhere-src-${NTRT_QT_VERSION}.tar.xz")
    FetchContent_GetProperties(ntrt_qt_source)
    if(NOT ntrt_qt_source_POPULATED)
        FetchContent_Populate(ntrt_qt_source)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -S "${ntrt_qt_source_SOURCE_DIR}"
        -B "${CMAKE_BINARY_DIR}/_deps/qt-build" -G Ninja
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        "-DCMAKE_INSTALL_PREFIX=${_local_qt}" -DCMAKE_BUILD_TYPE=Release
        -DBUILD_SHARED_LIBS=ON -DQT_BUILD_TESTS=OFF -DQT_BUILD_EXAMPLES=OFF
        COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}/_deps/qt-build" --parallel 4 COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}/_deps/qt-build" COMMAND_ERROR_IS_FATAL ANY)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets PATHS "${_local_qt}" NO_DEFAULT_PATH)
endif()
get_target_property(_qt_type Qt6::Core TYPE)
if(_qt_type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR "NTRT requires shared Qt; selected Qt is static")
endif()
