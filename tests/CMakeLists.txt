set(TEST_SOURCES
    test_listener.cpp
)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE nmea GTest::gtest_main GTest::gmock)

include(GoogleTest)
gtest_discover_tests(tests)

