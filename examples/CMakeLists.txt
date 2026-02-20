add_executable(listener listener.cpp)
target_link_libraries(listener PRIVATE nmea)
set_target_properties(listener PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

add_executable(device device.cpp)
target_link_libraries(device PRIVATE nmea)
set_target_properties(device PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
