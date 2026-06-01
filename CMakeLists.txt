cmake_minimum_required(VERSION 3.20)

project(MyProject)

set(CMAKE_CXX_STANDARD 20)

add_executable(MyProject
    main.cpp
     "tests.cpp"
    ThreadFilesReader.cpp
)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

FetchContent_MakeAvailable(googletest)

target_link_libraries(MyProject PRIVATE GTest::gtest)