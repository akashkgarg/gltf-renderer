if(TARGET argparse)
    return()
endif()

message(STATUS "Third-party: creating target 'argparse'")

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse
    GIT_TAG tags/v3.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(argparse)
