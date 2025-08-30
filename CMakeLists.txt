cmake_minimum_required(VERSION 3.16)
project(medialode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# nlohmann_json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Boost
find_package(Boost REQUIRED COMPONENTS system)

# Add subdirectories last
add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(lib/workers/image)
add_subdirectory(lib/workers/audio)
add_subdirectory(lib/workers/video)
add_subdirectory(app/libmgr-image-worker)
add_subdirectory(app/libmgr-audio-worker)
add_subdirectory(app/libmgr-video-worker)
