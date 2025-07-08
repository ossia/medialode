cmake_minimum_required(VERSION 3.15)
project(medialode LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include folders for headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(lib)
add_subdirectory(app)

# Optional: Add examples and tests
# add_subdirectory(examples/hello-world)
# add_subdirectory(tests)

