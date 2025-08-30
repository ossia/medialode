add_library(audio_worker STATIC
    audio_worker.cpp
)

target_include_directories(audio_worker
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_worker
    PRIVATE nlohmann_json::nlohmann_json
)

