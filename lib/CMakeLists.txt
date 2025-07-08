add_library(medialode-lib
    scanlib.cpp
)

target_include_directories(medialode-lib
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

# TODO: Add LLFIO, SQLite, Boost, etc. dependencies here
# target_link_libraries(medialode-lib PRIVATE ...)

