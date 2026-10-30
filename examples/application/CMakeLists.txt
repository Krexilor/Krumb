# TARGET -------------------------------------------------------------------------------------------------------------------------------------------|
add_executable(krumb_application application.cpp)
target_link_libraries(krumb_application PRIVATE krumb::krumb)

add_test(NAME krumb_application COMMAND krumb_application)
