cmake_minimum_required(VERSION 3.21)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are disabled. Use: cmake -S . -B build/release")
endif()
project(NTRT VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/dependencies/EnsureQt.cmake)
include(cmake/dependencies/EnsureSimdjson.cmake)
qt_standard_project_setup()

file(GLOB_RECURSE NTRT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.qrc")
qt_add_executable(NTRT ${NTRT_SOURCES})
target_include_directories(NTRT PRIVATE src)
target_link_libraries(NTRT PRIVATE Qt6::Widgets Qt6::Network Qt6::WebSockets simdjson::simdjson)
if(WIN32)
    set_target_properties(NTRT PROPERTIES WIN32_EXECUTABLE TRUE)

endif()
add_custom_target(ntrt_resources
    COMMAND "${CMAKE_COMMAND}"
        "-DRESOURCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
        "-DRESOURCE_DEST=$<TARGET_FILE_DIR:NTRT>/res"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/resources/CopyResources.cmake"
    COMMAND "${CMAKE_COMMAND}"
        "-DRESOURCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
        "-DRESOURCE_DEST=${CMAKE_BINARY_DIR}/dist/NTRT/res"
        "-DCONFIG=$<CONFIG>" -DEXISTING_PACKAGE_ONLY=ON
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/resources/CopyResources.cmake"
    VERBATIM)
add_dependencies(NTRT ntrt_resources)
if(WIN32)
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
    find_program(_ntrt_deploy_tool NAMES windeployqt6 windeployqt
        HINTS "${_qt_bin}" NO_DEFAULT_PATH NO_CACHE REQUIRED)
    get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(_vc_install_dir "${NTRT_VC_INSTALL_DIR}")
    if(CMAKE_VS_INSTALLATION_PATH)
        set(_vc_install_dir "${CMAKE_VS_INSTALLATION_PATH}/VC")
    elseif(NOT _vc_install_dir)
        set(_vc_install_dir "$ENV{VCINSTALLDIR}")
    endif()
    set(_qt_tls "")
    if(TARGET Qt6::QSchannelBackendPlugin)
        set(_qt_tls "$<TARGET_FILE:Qt6::QSchannelBackendPlugin>")
    endif()
    # Deploy development DLLs once per Qt installation, rather than every relink.
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runtime-$<CONFIG>-ready.stamp"
        COMMAND "${CMAKE_COMMAND}" "-DDEST=$<TARGET_FILE_DIR:NTRT>"
            "-DMINGW=${MINGW}"
            "-DQT_CORE=$<TARGET_FILE:Qt6::Core>" "-DQT_GUI=$<TARGET_FILE:Qt6::Gui>"
            "-DQT_WIDGETS=$<TARGET_FILE:Qt6::Widgets>"
            "-DQT_NETWORK=$<TARGET_FILE:Qt6::Network>"
            "-DQT_WEBSOCKETS=$<TARGET_FILE:Qt6::WebSockets>"
            "-DQT_BIN=$<TARGET_FILE_DIR:Qt6::Core>"
            "-DQT_PLATFORM=$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
            "-DQT_TLS=${_qt_tls}"
            "-DQT_ICO=$<TARGET_FILE:Qt6::QICOPlugin>"
            "-DRUNTIME_BIN=${_compiler_bin}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy/PrepareRuntime.cmake"
        COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/runtime-$<CONFIG>-ready.stamp"
        DEPENDS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets
            Qt6::QICOPlugin
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy/PrepareRuntime.cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy/DeployRuntime.cmake"
        VERBATIM)
    add_custom_target(ntrt_runtime DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/runtime-$<CONFIG>-ready.stamp")
    add_dependencies(NTRT ntrt_runtime)
    add_custom_command(TARGET NTRT POST_BUILD
        COMMAND "${CMAKE_COMMAND}" "-DCONFIG=$<CONFIG>"
            "-DAPP=$<TARGET_FILE:NTRT>" "-DBUILD_ROOT=${CMAKE_BINARY_DIR}"
            "-DRESOURCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
            "-DQT_BIN=$<TARGET_FILE_DIR:Qt6::Core>"
            "-DVC_INSTALL_DIR=${_vc_install_dir}"
            "-DMINGW=${MINGW}" "-DDEPLOY_TOOL=${_ntrt_deploy_tool}" "-DRUNTIME_BIN=${_compiler_bin}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy/PackageRelease.cmake"
        VERBATIM)
endif()
if(MSVC)
    target_compile_options(NTRT PRIVATE /W4 /utf-8 /Zc:__cplusplus)
else()
    target_compile_options(NTRT PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(NTRT_BUILD_TESTS "Build candle data and settings tests" OFF)
if(NTRT_BUILD_TESTS)
    enable_testing()
    qt_add_executable(NTRTCandlesTests
        tests/CandlesTests.cpp
        src/main/monitor/CandlesManager.cpp
            src/main/trade/parser/OKXParser.cpp
        src/main/data/AppearanceManager.cpp
        src/main/data/ConfigManager.cpp
        src/main/util/JsonUtil.cpp
        src/main/util/FileUtil.cpp)
    target_include_directories(NTRTCandlesTests PRIVATE src)
    target_link_libraries(NTRTCandlesTests PRIVATE Qt6::Widgets Qt6::Network simdjson::simdjson)
    if(TARGET ntrt_runtime)
        add_dependencies(NTRTCandlesTests ntrt_runtime)
    endif()
    add_test(NAME candles COMMAND NTRTCandlesTests)
endif()
