file(MAKE_DIRECTORY "${DEST}/platforms")
file(COPY "${QT_CORE}" "${QT_GUI}" "${QT_WIDGETS}" DESTINATION "${DEST}")
file(COPY "${QT_PLATFORM}" DESTINATION "${DEST}/platforms")
# Scan only these libraries: there is no application executable on a fresh build.
set(APP "")
if(MINGW)
    include("${CMAKE_CURRENT_LIST_DIR}/DeployRuntime.cmake")
endif()
