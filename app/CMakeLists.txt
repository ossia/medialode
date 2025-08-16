# Make Boost and nlohmann_json available
find_package(Boost REQUIRED COMPONENTS system thread)

# Ensure this includes the header path for ipc/protocol.hpp
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Main app
add_executable(medialode-app main.cpp)
target_link_libraries(medialode-app PRIVATE medialode-lib)

# Ping server
add_executable(ping-server ping-server.cpp)
target_link_libraries(ping-server
  PRIVATE medialode-lib
          Boost::system Boost::thread
          nlohmann_json::nlohmann_json
)

# Ping client
add_executable(ping-client ping-client.cpp)
target_link_libraries(ping-client
  PRIVATE medialode-lib
          nlohmann_json::nlohmann_json
)

target_compile_definitions(ping-client PRIVATE
                           ASIO_STANDALONE=0
                           _WEBSOCKETPP_CPP11_INTERNAL_

)
