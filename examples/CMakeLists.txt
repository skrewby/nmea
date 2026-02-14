add_executable(listener listener.cpp)
target_link_libraries(listener PRIVATE nmea)
set_target_properties(listener PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
