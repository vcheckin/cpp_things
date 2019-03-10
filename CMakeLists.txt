cmake_minimum_required(VERSION 3.10)
project(cpp_things)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(tests)
add_subdirectory(src)
enable_testing()
add_test(NAME tests COMMAND tests)

