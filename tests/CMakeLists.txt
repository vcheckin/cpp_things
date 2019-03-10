find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(tests ptr_tests.cpp)
target_link_libraries(tests 
  cpp_things 
  GTest::gtest_main)