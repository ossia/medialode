cmake_minimum_required(VERSION 3.16)
project(medialode LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
include(FetchContent)

# --- nlohmann_json ---
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- Boost (header-only for Asio/Beast) ---
find_package(Boost REQUIRED COMPONENTS system)

# --- OSSIA ---
FetchContent_Declare(
  ossia
  GIT_REPOSITORY https://github.com/ossia/libossia.git
  GIT_TAG master
)

include(FetchContent)

# WebSocketPP
FetchContent_Declare(
  websocketpp
  GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(websocketpp)

# nano-signal-slot
FetchContent_Declare(
  nano_signal_slot
  GIT_REPOSITORY https://github.com/NoAvailableAlias/nano-signal-slot.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(nano_signal_slot)

# Disable unused OSSIA features to speed up build
set(OSSIA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OSSIA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(OSSIA_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
set(OSSIA_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
set(OSSIA_BUILD_JAVA OFF CACHE BOOL "" FORCE)
set(OSSIA_BUILD_MAX OFF CACHE BOOL "" FORCE)
set(OSSIA_BUILD_PD OFF CACHE BOOL "" FORCE)
set(OSSIA_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(OSSIA_SUPERBUILD OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ossia)

target_compile_definitions(ping-client PRIVATE ASIO_STANDALONE=0)

# Add lib and app directories
add_subdirectory(lib)
add_subdirectory(app)
