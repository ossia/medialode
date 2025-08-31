find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil)

add_executable(libmgr-video-worker
    main.cpp
)
target_include_directories(libmgr-video-worker PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(libmgr-video-worker
    PRIVATE
        video_worker
        Boost::system
        PkgConfig::FFMPEG
)

