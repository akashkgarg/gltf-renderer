message(STATUS "Third-party: downloading 'filament'")

include(FetchContent)
if (TARGET_OS_MACOS)
FetchContent_Declare(
  filament 
  URL https://github.com/google/filament/releases/download/v1.54.4/filament-v1.54.4-mac.tgz
  URL_HASH MD5=f2938610b44da779ffe394d8c55b5b5d
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
elseif (TARGET_OS_LINUX)
FetchContent_Declare(
  filament 
  URL https://github.com/google/filament/releases/download/v1.56.7/filament-v1.56.7-linux.tgz
  URL_HASH MD5=3c49ca1dbe78f56eb94fda44ac95c455
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
endif()

FetchContent_MakeAvailable(filament)

message("Filament extracted to: ${filament_SOURCE_DIR}")