include_directories(".")

find_package(Boost CONFIG REQUIRED
        COMPONENTS
        date_time
        filesystem
        iostreams
        thread
)

add_executable(${PROJECT_NAME}
  main.cpp
)

target_link_libraries(${PROJECT_NAME}
    jmixin
    jinject
        jCanvas::Core
    fmt
    expected
    crypt
    Boost::date_time
    Boost::filesystem
    Boost::iostreams
    Boost::thread
)
