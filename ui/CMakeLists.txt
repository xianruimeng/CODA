project(coda-ui)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
if (${BUILD_ui})
add_subdirectory(src/common)
add_subdirectory(src/client)
add_subdirectory(src/server)
endif (${BUILD_ui})
