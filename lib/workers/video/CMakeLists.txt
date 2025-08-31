add_library(video_worker STATIC
    video_worker.cpp
)

target_include_directories(video_worker
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(video_worker
    PUBLIC nlohmann_json::nlohmann_json
)


