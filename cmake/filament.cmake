if (APPLE)
message(STATUS "Third-party: downloading 'filament'")

include(FetchContent)
FetchContent_Declare(
  filament 
  URL https://github.com/google/filament/releases/download/v1.54.4/filament-v1.54.4-mac.tgz
  URL_HASH MD5=f2938610b44da779ffe394d8c55b5b5d
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(filament)

message("Filament extracted to: ${filament_SOURCE_DIR}")
endif()
