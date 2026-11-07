# MSYS2 Qt also depends on non-Qt DLLs which windeployqt does not copy.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${RUNTIME_BIN}/objdump.exe")
if(APP)
    get_filename_component(destination "${APP}" DIRECTORY)
else()
    set(destination "${DEST}")
endif()
file(GLOB_RECURSE deployed_libraries "${destination}/*.dll")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${APP}
    LIBRARIES ${deployed_libraries}
    DIRECTORIES "${RUNTIME_BIN}"
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*"
    RESOLVED_DEPENDENCIES_VAR dependencies
    CONFLICTING_DEPENDENCIES_PREFIX conflicts
    UNRESOLVED_DEPENDENCIES_VAR missing)
if(missing)
    message(FATAL_ERROR "Unresolved runtime libraries: ${missing}")
endif()
foreach(name IN LISTS conflicts_FILENAMES)
    if(EXISTS "${RUNTIME_BIN}/${name}")
        file(COPY "${RUNTIME_BIN}/${name}" DESTINATION "${destination}")
    else()
        message(FATAL_ERROR "Conflicting runtime library: ${name}")
    endif()
endforeach()
foreach(dependency IN LISTS dependencies)
    get_filename_component(source_dir "${dependency}" DIRECTORY)
    if(NOT source_dir STREQUAL destination)
        file(COPY "${dependency}" DESTINATION "${destination}")
    endif()
endforeach()
