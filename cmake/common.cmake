# don't build in the source directory
if ("${CMAKE_BINARY_DIR}/.." STREQUAL "${CMAKE_SOURCE_DIR}" OR
    "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}/csim")
  message (SEND_ERROR "Do not build in the source directory.")
  message (SEND_ERROR "Use ${CMAKE_SOURCE_DIR}/build instead.")
  message (FATAL_ERROR "Remove the created \"CMakeCache.txt\" file and the \
           \"CMakeFiles\" directory, then create a build directory and call \
           \"${CMAKE_COMMAND}   <path to the sources>\".")
endif ("${CMAKE_BINARY_DIR}/.." STREQUAL "${CMAKE_SOURCE_DIR}" OR
       "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}/csim")

# finds all files with a given extension
macro (append_files files ext)
  foreach (dir ${ARGN})
    file (GLOB _files "${dir}/*.${ext}")
    list (APPEND ${files} ${_files})
  endforeach (dir)
endmacro (append_files)
