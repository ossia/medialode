add_executable(libmgr-image-worker
    main.cpp
)

target_include_directories(libmgr-image-worker PRIVATE ${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(libmgr-image-worker
    PRIVATE image_worker
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE Boost::system
)

