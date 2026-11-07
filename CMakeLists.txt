cmake_minimum_required(VERSION 3.21)
project(NTRT VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(cmake/EnsureQt.cmake)
qt_standard_project_setup()

qt_add_executable(NTRT
        src/main/NTRT.cpp
        src/main/NTRT.h
        src/main/ui/MainWindow.cpp
        src/main/ui/MainWindow.h
        src/main/utils/FileUtils.h
        src/main/utils/FileUtils.cpp
        src/main/Ver.h
)
target_include_directories(NTRT PRIVATE src)
target_link_libraries(NTRT PRIVATE Qt6::Widgets)
add_custom_target(ntrt_resources
    COMMAND "${CMAKE_COMMAND}"
        "-DRESOURCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
        "-DRESOURCE_DEST=$<TARGET_FILE_DIR:NTRT>/res"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyResources.cmake"
    COMMAND "${CMAKE_COMMAND}"
        "-DRESOURCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
        "-DRESOURCE_DEST=${CMAKE_BINARY_DIR}/dist/NTRT/res"
        "-DCONFIG=$<CONFIG>" -DEXISTING_PACKAGE_ONLY=ON
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyResources.cmake"
    VERBATIM)
add_dependencies(NTRT ntrt_resources)
if(WIN32 AND MINGW)
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qt_qmake}" DIRECTORY)
    find_program(NTRT_WINDEPLOYQT NAMES windeployqt6 windeployqt
        HINTS "${_qt_bin}" REQUIRED)
    get_filename_component(_compiler_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    # Deploy development DLLs once per Qt installation, rather than every relink.
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runtime-ready.stamp"
        COMMAND "${CMAKE_COMMAND}" "-DDEST=${CMAKE_CURRENT_BINARY_DIR}"
            "-DQT_CORE=$<TARGET_FILE:Qt6::Core>" "-DQT_GUI=$<TARGET_FILE:Qt6::Gui>"
            "-DQT_WIDGETS=$<TARGET_FILE:Qt6::Widgets>"
            "-DQT_PLUGINS=${_qt_bin}/../share/qt6/plugins"
            "-DRUNTIME_BIN=${_compiler_bin}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareRuntime.cmake"
        COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_CURRENT_BINARY_DIR}/runtime-ready.stamp"
        DEPENDS Qt6::Core Qt6::Gui Qt6::Widgets
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PrepareRuntime.cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployRuntime.cmake"
        VERBATIM)
    add_custom_target(ntrt_runtime DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/runtime-ready.stamp")
    add_dependencies(NTRT ntrt_runtime)
    add_custom_command(TARGET NTRT POST_BUILD
        COMMAND "${CMAKE_COMMAND}" "-DCONFIG=$<CONFIG>"
            "-DAPP=$<TARGET_FILE:NTRT>" "-DBUILD_ROOT=${CMAKE_BINARY_DIR}"
            "-DRESOURCE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
            "-DDEPLOY_TOOL=${NTRT_WINDEPLOYQT}" "-DRUNTIME_BIN=${_compiler_bin}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageRelease.cmake"
        VERBATIM)
endif()
if(MSVC)
    target_compile_options(NTRT PRIVATE /W4 /utf-8)
else()
    target_compile_options(NTRT PRIVATE -Wall -Wextra -Wpedantic)
endif()
