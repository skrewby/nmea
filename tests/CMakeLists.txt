set(TEST_SOURCES
    test_listener.cpp
    test_address_claiming.cpp
    test_serialization.cpp
    test_device_send.cpp
)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE nmea GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
