add_executable(medialode-app
    main.cpp
)

target_link_libraries(medialode-app
    PRIVATE medialode-lib
)

