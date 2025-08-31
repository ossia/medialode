add_executable(libmgr-cli
  main.cpp
)

target_include_directories(libmgr-cli PRIVATE
  ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(libmgr-cli
  Boost::system
  nlohmann_json::nlohmann_json
)

