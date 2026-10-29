# TARGET -------------------------------------------------------------------------------------------------------------------------------------------|
find_package(Threads REQUIRED)

add_executable(krumb_example example.cpp)
target_link_libraries(krumb_example PRIVATE krumb::krumb Threads::Threads)

add_test(NAME krumb_example COMMAND krumb_example)
