add_library(image_worker STATIC
    image_worker.cpp
)

target_include_directories(image_worker
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_SOURCE_DIR}/lib/deps/stb
)

target_link_libraries(image_worker
    PUBLIC nlohmann_json::nlohmann_json
)

