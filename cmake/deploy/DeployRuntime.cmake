# MSYS2 Qt also depends on non-Qt DLLs which windeployqt does not copy.
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
find_program(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND NAMES objdump llvm-objdump
    HINTS "${RUNTIME_BIN}" NO_DEFAULT_PATH REQUIRED)
if(APP)
    get_filename_component(destination "${APP}" DIRECTORY)
else()
    set(destination "${DEST}")
endif()
# Only the release package is safe to scan recursively. Development builds
# also contain dependency SDKs, which are not part of the application.
if(APP)
    file(GLOB_RECURSE deployed_libraries "${destination}/*.dll")
endif()
set(runtime_dirs "${destination}" "${QT_BIN}" "${RUNTIME_BIN}")
list(FILTER runtime_dirs EXCLUDE REGEX "^$")
list(REMOVE_DUPLICATES runtime_dirs)
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${APP}
    LIBRARIES ${deployed_libraries}
    DIRECTORIES ${runtime_dirs}
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*"
    RESOLVED_DEPENDENCIES_VAR dependencies
    CONFLICTING_DEPENDENCIES_PREFIX conflicts
    UNRESOLVED_DEPENDENCIES_VAR missing)
if(missing)
    message(FATAL_ERROR "Unresolved runtime libraries: ${missing}")
endif()
foreach(name IN LISTS conflicts_FILENAMES)
    set(chosen "")
    foreach(directory IN LISTS runtime_dirs)
        if(EXISTS "${directory}/${name}")
            set(chosen "${directory}/${name}")
            break()
        endif()
    endforeach()
    if(NOT chosen)
        message(FATAL_ERROR "Conflicting runtime library: ${name}")
    endif()
    if(NOT chosen STREQUAL "${destination}/${name}")
        file(COPY "${chosen}" DESTINATION "${destination}")
    endif()
endforeach()
foreach(dependency IN LISTS dependencies)
    get_filename_component(source_dir "${dependency}" DIRECTORY)
    if(NOT source_dir STREQUAL destination)
        file(COPY "${dependency}" DESTINATION "${destination}")
    endif()
endforeach()
