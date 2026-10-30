# PROJECT ------------------------------------------------------------------------------------------------------------------------------------------|
cmake_minimum_required(VERSION 3.21)

project(Krumb
    VERSION 0.1.0
    DESCRIPTION "A small, modern C++20 logging library"
    LANGUAGES CXX)

# OPTIONS ------------------------------------------------------------------------------------------------------------------------------------------|
option(KRUMB_BUILD_TESTS "Build Krumb tests" ${PROJECT_IS_TOP_LEVEL})
option(KRUMB_BUILD_EXAMPLES "Build Krumb examples" ${PROJECT_IS_TOP_LEVEL})
option(KRUMB_WARNINGS_AS_ERRORS "Treat warnings as errors when building Krumb" OFF)
option(KRUMB_ENABLE_TSAN "Build Krumb, its tests and its examples with ThreadSanitizer" OFF)

# TARGET -------------------------------------------------------------------------------------------------------------------------------------------|
add_library(krumb STATIC)
add_library(krumb::krumb ALIAS krumb)

# SOURCES ------------------------------------------------------------------------------------------------------------------------------------------|
target_sources(krumb PRIVATE
    src/formatters/text_formatter.cpp
    src/sinks/console_sink.cpp
    src/sinks/file_sink.cpp
    src/logger.cpp
)

# INCLUDES -----------------------------------------------------------------------------------------------------------------------------------------|
target_include_directories(krumb
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

# STANDARD -----------------------------------------------------------------------------------------------------------------------------------------|
target_compile_features(krumb PUBLIC cxx_std_20)
set_target_properties(krumb PROPERTIES CXX_EXTENSIONS OFF)

# WARNINGS -----------------------------------------------------------------------------------------------------------------------------------------|
if(MSVC)
    target_compile_options(krumb PRIVATE /W4
        $<$<BOOL:${KRUMB_WARNINGS_AS_ERRORS}>:/WX>)
else()
    target_compile_options(krumb PRIVATE -Wall -Wextra -Wpedantic
        $<$<BOOL:${KRUMB_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

# SANITIZERS ---------------------------------------------------------------------------------------------------------------------------------------|
if(KRUMB_ENABLE_TSAN)
    if(MSVC)
        message(FATAL_ERROR "KRUMB_ENABLE_TSAN is not supported with MSVC")
    endif()

    # ThreadSanitizer needs every part of the program built with it. BUILD_INTERFACE keeps the flags out of an installed package.
    target_compile_options(krumb
        PUBLIC
            $<BUILD_INTERFACE:-fsanitize=thread>
            $<BUILD_INTERFACE:-fno-omit-frame-pointer>)
    target_link_options(krumb
        PUBLIC
            $<BUILD_INTERFACE:-fsanitize=thread>)
endif()

# TESTS AND EXAMPLES -------------------------------------------------------------------------------------------------------------------------------|
if(KRUMB_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

if(KRUMB_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()
