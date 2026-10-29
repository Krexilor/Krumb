# TARGET -------------------------------------------------------------------------------------------------------------------------------------------|
add_executable(krumb_tests
    unit/main.cpp
    unit/test_level.cpp
    unit/test_format_string.cpp
    unit/test_text_formatter.cpp
    unit/test_logger.cpp
)

target_link_libraries(krumb_tests PRIVATE krumb::krumb)
target_include_directories(krumb_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# WARNINGS -----------------------------------------------------------------------------------------------------------------------------------------|
if(MSVC)
    target_compile_options(krumb_tests PRIVATE /W4)
else()
    target_compile_options(krumb_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# TEST REGISTRATION --------------------------------------------------------------------------------------------------------------------------------|
add_test(NAME krumb_tests COMMAND krumb_tests)
