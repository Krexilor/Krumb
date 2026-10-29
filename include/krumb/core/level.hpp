#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cstdint>
#include <string_view>

// LEVELS ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // severity levels, ordered from least to most severe, so levels can be compared directly.
    // `off` is only meant as a filter threshold, never as the level of a message.
    enum class level : std::uint8_t {
        trace,
        debug,
        info,
        warn,
        error,
        critical,
        off
    };

    constexpr std::string_view to_string(level lvl) noexcept {
        switch (lvl) {
            case level::trace:
                return "trace";

            case level::debug:
                return "debug";

            case level::info:
                return "info";

            case level::warn:
                return "warn";

            case level::error:
                return "error";

            case level::critical:
                return "critical";

            case level::off:
                return "off";
        }

        return "unknown";
    }
}
