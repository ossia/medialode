include(FetchContent)

# OUTCOME
FetchContent_Declare(
  outcome
  GIT_REPOSITORY https://github.com/ned14/outcome.git
  GIT_TAG        744da6b7536f2850df972ab01504e3c4d9530149
)
FetchContent_MakeAvailable(outcome)

# QUICKCPPLIB
FetchContent_Declare(
  quickcpplib
  GIT_REPOSITORY https://github.com/ned14/quickcpplib.git
  GIT_TAG        659f470cec041df6c7d74c5786d9c518dc42263e
)
FetchContent_MakeAvailable(quickcpplib)

# LLFIO
FetchContent_Declare(
  llfio
  GIT_REPOSITORY https://github.com/ned14/llfio.git
  GIT_TAG        af19b0da98b5b8c834a40a421a35b5a581e3e312
)
FetchContent_MakeAvailable(llfio)

add_library(medialode-lib
    scanlib.cpp
    file_watcher.cpp
)

target_include_directories(medialode-lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${llfio_SOURCE_DIR}/include
        ${outcome_SOURCE_DIR}/include
        ${quickcpplib_SOURCE_DIR}/include
)

target_link_libraries(medialode-lib
    PUBLIC llfio_sl
)
