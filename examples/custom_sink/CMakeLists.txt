# TARGET -------------------------------------------------------------------------------------------------------------------------------------------|
add_executable(krumb_custom_sink custom_sink.cpp)
target_link_libraries(krumb_custom_sink PRIVATE krumb::krumb)

add_test(NAME krumb_custom_sink COMMAND krumb_custom_sink)
