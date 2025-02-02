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
include (filament)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Packages
#------------------------------------------------------------------------------ 

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

# do we need all of these?
set(FILAMENT_LIBS
  meshoptimizer
  geometry
  basis_transcoder
  ibl
  backend
  bluegl
  filamat
  filament
  filament-iblprefilter
  filameshio
  filabridge
  filaflat
  bluevk
  dracodec
  smol-v
  gltfio_core
  gltfio
  utils
  image
  ktxreader
  uberarchive
  zstd
  uberzlib)

add_executable(${PROJECT_NAME} ${Sources} ${NativeSources})
target_link_libraries(${PROJECT_NAME} ${FILAMENT_LIBS} ${FRAMEWORKS})

# make sure we can access assets from build dir
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${PROJECT_SOURCE_DIR}/assets
                ${CMAKE_BINARY_DIR}/assets)

