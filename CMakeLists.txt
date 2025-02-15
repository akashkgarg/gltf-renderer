cmake_minimum_required(VERSION 3.22)

# version number and definitions 
#-------------------------------------------------------------------------------

project(gltf-renderer VERSION 0.1 LANGUAGES CXX C)

# Extra modules for cmake
#-------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_PREFIX_PATH})

# add common cmake file
include (common)
include (TargetOS)

if (TARGET_OS_MACOS)
include (filament)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Packages
#------------------------------------------------------------------------------ 

if (TARGET_OS_LINUX)
  set(CMAKE_CXX_FLAGS "-pthread -fPIC -m64 -stdlib=libc++")
  set(filament_SOURCE_DIR /home/jovyan/data/akash/filament-1.56.7/out/release/filament)
endif()

include_directories(src
  SYSTEM ${filament_SOURCE_DIR}/include)

link_directories(
  ${filament_SOURCE_DIR}/lib/${CMAKE_HOST_SYSTEM_PROCESSOR})

# Sources
#------------------------------------------------------------------------------

set (Directories
  src/
)

append_files(Sources "cpp" ${Directories})
                                                                                 
set(GENERATION_ROOT ${CMAKE_CURRENT_BINARY_DIR})
set(RESOURCE_DIR  "${GENERATION_ROOT}/generated/resources")
set(MATERIAL_DIR  "${GENERATION_ROOT}/generated/material")

# Targets
#------------------------------------------------------------------------------

if (TARGET_OS_MACOS)
set(FRAMEWORKS 
    "-framework Cocoa"
    "-framework Metal"
    "-framework CoreVideo"
    "-framework IOKit"
    "-framework QuartzCore")
endif()

if (TARGET_OS_LINUX)
set(LINUX_LIBS
  dl
  c++
  c++abi)
endif()

# do we need all of these?
set(FILAMENT_LIBS
  utils
  filament 
  backend 
  meshoptimizer
  mikktspace
  geometry 
  bluevk 
  bluegl
  filabridge 
  filaflat 
  smol-v 
  filament-iblprefilter
  ktxreader
  ibl
  image
  zstd
  gltfio
  gltfio_core
  uberarchive
  uberzlib
  dracodec
  basis_transcoder
)

add_executable(${PROJECT_NAME} ${Sources} ${NativeSources})
target_link_libraries(${PROJECT_NAME} ${LINUX_LIBS} ${FILAMENT_LIBS} ${FILAMENT_LIBS})

# make sure we can access assets from build dir
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${PROJECT_SOURCE_DIR}/assets
                ${CMAKE_BINARY_DIR}/assets)

