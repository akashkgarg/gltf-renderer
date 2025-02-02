if(TARGET earcut_hpp)
    return()
endif()


message(STATUS "Third-party: creating target 'earcut_hpp'")

include(FetchContent)
FetchContent_Declare(
    earcut_hpp
    GIT_REPOSITORY https://github.com/mapbox/earcut.hpp.git
    GIT_TAG tags/v2.2.4
    GIT_SHALLOW TRUE
    CMAKE_ARGS -DEARCUT_BUILD_VIZ:BOOL=OFF -DEARCUT_BUILD_TESTS:BOOL=OFF -DEARCUT_BUILD_BENCH:BOOL=OFF
)
FetchContent_MakeAvailable(earcut_hpp)

