# Dependencies
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(SQLite3 REQUIRED)

# CLI client (scan-folders-cli)
add_executable(scan-folders-cli scan-folders-cli.cpp)
target_include_directories(scan-folders-cli PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(scan-folders-cli
  PRIVATE
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
)

# libmgr-server (WebSocket server)
add_executable(libmgr-server libmgr-server.cpp)
target_include_directories(libmgr-server PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(libmgr-server
  PRIVATE
    Boost::system
    Boost::thread
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
)

#Generic Scanner Worker
add_executable(libmgr-scan-worker libmgr-scan-worker.cpp)
target_include_directories(libmgr-scan-worker PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(libmgr-scan-worker
  PRIVATE
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
)

# scan-file-cli
add_executable(scan-file-cli
  scan-file-cli.cpp
)
target_include_directories(scan-file-cli PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(scan-file-cli
  PRIVATE
    nlohmann_json::nlohmann_json
    Boost::system
    Boost::thread
)

