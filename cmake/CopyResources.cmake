if(EXISTING_PACKAGE_ONLY)
    get_filename_component(package_dir "${RESOURCE_DEST}" DIRECTORY)
    if(NOT CONFIG STREQUAL "Release" OR NOT EXISTS "${package_dir}/NTRT.exe")
        return()
    endif()
endif()
file(MAKE_DIRECTORY "${RESOURCE_DEST}")
if(IS_DIRECTORY "${RESOURCE_SOURCE}")
    # The trailing slash copies the contents, preserving subdirectories.
    # file(COPY) skips files whose timestamps and sizes already match.
    file(COPY "${RESOURCE_SOURCE}/" DESTINATION "${RESOURCE_DEST}")
endif()
