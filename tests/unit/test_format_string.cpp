// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <format>
#include <string>
#include <cstdint>
#include <utility>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/format_string.hpp>

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    std::uint_least32_t captured_line(krumb::location_format_string<> fmt) {
        return fmt.location.line();
    }

    std::string_view captured_file(krumb::location_format_string<> fmt) {
        return fmt.location.file_name();
    }

    template <typename... Args>
    std::string render(krumb::location_format_string<Args...> fmt, Args&&... args) {
        return std::format(fmt.fmt, std::forward<Args>(args)...);
    }
}

// FORMAT STRING TESTS -----------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("call-site line is captured") {
    CHECK(captured_line("message") == std::source_location::current().line());
}

TEST_CASE("call-site file is captured") {
    CHECK(captured_file("message") == std::string_view{std::source_location::current().file_name()});
}

TEST_CASE("arguments are formatted") {
    CHECK(render("no arguments") == "no arguments");
    CHECK(render("x = {}, name = {}", 42, "krumb") == "x = 42, name = krumb");
    CHECK(render("pi = {:.2f}", 3.14159) == "pi = 3.14");
    CHECK(render("{0}-{0}", 7) == "7-7");
}
