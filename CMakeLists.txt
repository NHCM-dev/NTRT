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
target_link_libraries(NTRT PRIVATE Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::OpenGL Qt6::OpenGLWidgets simdjson::simdjson)
if(WIN32)
    enable_language(RC)
    set(NTRT_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon.ico")
    configure_file(cmake/resources/WindowsIcon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/WindowsIcon.rc" @ONLY)
    target_sources(NTRT PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/WindowsIcon.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/WindowsIcon.rc" PROPERTIES OBJECT_DEPENDS "${NTRT_ICON_PATH}")
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
            "-DQT_OPENGL=$<TARGET_FILE:Qt6::OpenGL>" "-DQT_OPENGLWIDGETS=$<TARGET_FILE:Qt6::OpenGLWidgets>"
            "-DQT_BIN=$<TARGET_FILE_DIR:Qt6::Core>"
            "-DQT_PLATFORM=$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
            "-DQT_TLS=${_qt_tls}"
            "-DQT_ICO=$<TARGET_FILE:Qt6::QICOPlugin>"
            "-DRUNTIME_BIN=${_compiler_bin}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy/PrepareRuntime.cmake"
        COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/runtime-$<CONFIG>-ready.stamp"
        DEPENDS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::OpenGL Qt6::OpenGLWidgets
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

option(NTRT_BUILD_TESTS "Build local protocol and transport tests" OFF)
if(NTRT_BUILD_TESTS)
    enable_testing()
    qt_add_executable(OKXAccountTest tests/OKXAccountTest.cpp
        src/main/util/exchange/OKXAccountUtil.cpp src/main/trade/parser/OKXParser.cpp
        src/main/util/JsonUtil.cpp src/main/util/FileUtil.cpp
        src/main/network/RestAPITemplate.cpp src/main/util/NetworkUtil.cpp
        src/main/util/WorkspaceStateUtil.cpp)
    target_include_directories(OKXAccountTest PRIVATE src)
    target_link_libraries(OKXAccountTest PRIVATE Qt6::Widgets Qt6::Network simdjson::simdjson)
    if(MSVC)
        target_compile_options(OKXAccountTest PRIVATE /W4 /utf-8 /Zc:__cplusplus)
    endif()
    if(TARGET ntrt_runtime)
        add_dependencies(OKXAccountTest ntrt_runtime)
    endif()
    add_test(NAME okx_account COMMAND OKXAccountTest)
    set_tests_properties(okx_account PROPERTIES TIMEOUT 10)
    qt_add_executable(SelectionTableTest tests/SelectionTableTest.cpp src/main/ui/widgets/SelectionTable.cpp src/main/ui/widgets/SelectionTable.h)
    target_include_directories(SelectionTableTest PRIVATE src)
    target_link_libraries(SelectionTableTest PRIVATE Qt6::Widgets)
    if(MSVC)
        target_compile_options(SelectionTableTest PRIVATE /W4 /utf-8 /Zc:__cplusplus)
    endif()
    if(TARGET ntrt_runtime)
        add_dependencies(SelectionTableTest ntrt_runtime)
    endif()
    add_test(NAME selection_table COMMAND SelectionTableTest)
    get_target_property(selection_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(selection_qt_bin "${selection_qmake}" DIRECTORY)
    set_tests_properties(selection_table PROPERTIES TIMEOUT 10 ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${selection_qt_bin}/../plugins/platforms")
endif()
