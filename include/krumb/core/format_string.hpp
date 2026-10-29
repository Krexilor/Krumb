#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <format>
#include <concepts>
#include <string_view>
#include <type_traits>
#include <source_location>

// FORMAT STRING -----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    template <typename... Args>
    struct format_with_location {
        std::format_string<Args...> fmt;
        std::source_location location;

        template <typename String>
            requires std::convertible_to<const String&, std::string_view>
        consteval format_with_location(const String& str, std::source_location loc = std::source_location::current())
            : fmt(str),
              location(loc) {
        }
    };

    template <typename... Args>
    using location_format_string = format_with_location<std::type_identity_t<Args>...>;
}
