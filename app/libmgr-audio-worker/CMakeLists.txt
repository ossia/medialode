find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
)

add_executable(libmgr-audio-worker main.cpp)
target_include_directories(libmgr-audio-worker PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(libmgr-audio-worker
    PRIVATE audio_worker
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE Boost::system
    PkgConfig::FFMPEG
)


