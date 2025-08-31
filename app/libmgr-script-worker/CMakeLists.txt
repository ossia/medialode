add_executable(libmgr-script-worker
    main.cpp
)

target_include_directories(libmgr-script-worker
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(libmgr-script-worker
    PRIVATE
        script_worker
        Boost::system
        nlohmann_json::nlohmann_json
)


