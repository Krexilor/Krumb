#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <chrono>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/level.hpp>

// EVENT -------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // a single log record passed from the logger to the formatter and the sinks.
    struct event {
        level lvl;
        std::chrono::system_clock::time_point timestamp;
        std::source_location location;
        std::string_view message;
    };
}
