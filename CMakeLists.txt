cmake_minimum_required(VERSION 3.20)
project(nmea)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (LIBRARY_SOURCES
    src/connection.cpp
    src/listener.cpp
    src/message.cpp
)

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
)

option(NMEA_BUILD_EXAMPLES "Build examples" ON)
if(NMEA_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(NMEA_BUILD_TESTS "Build tests" ON)
if(NMEA_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()
