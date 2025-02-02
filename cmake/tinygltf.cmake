if(TARGET TinyGLTF)
    return()
endif()


message(STATUS "Third-party: creating target 'tinygltf'")

include(FetchContent)
FetchContent_Declare(
    tinygltf 
    GIT_REPOSITORY git@github.com:geomagical/tinygltf.git
    GIT_TAG 348d67372d90670ae487f6053484137a8f898b67
    GIT_SHALLOW TRUE
    CMAKE_ARGS -DTINYGLTF_HEADER_ONLY
)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF)
FetchContent_MakeAvailable(tinygltf)

include_directories(${tinygltf_SOURCE_DIR})