add_library(script_worker STATIC
    script_worker.cpp
)

target_include_directories(script_worker
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(script_worker
    PRIVATE nlohmann_json::nlohmann_json
)
