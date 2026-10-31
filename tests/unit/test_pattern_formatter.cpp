// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <string>
#include <chrono>
#include <format>
#include <stdexcept>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/event.hpp>
#include <krumb/formatters/pattern_formatter.hpp>

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // Builds an event with a fixed timestamp, so the output is predictable.
    krumb::event make_event(krumb::level lvl, std::string_view message) {
        using namespace std::chrono_literals;

        return krumb::event{
            .lvl = lvl,
            .timestamp = std::chrono::sys_days{2026y / 10 / 29} + 12h + 34min + 56s + 789ms,
            .location = std::source_location::current(),
            .message = message
        };
    }

    // Formats one event with the pattern.
    std::string render(std::string_view pattern, const krumb::event& ev) {
        krumb::pattern_formatter formatter{pattern};
        std::string out;

        formatter.format(ev, out);

        return out;
    }

    // The error text of a pattern that is rejected, or an empty string if it is accepted.
    std::string rejection(std::string_view pattern) {
        try {
            krumb::pattern_formatter formatter{pattern};

            static_cast<void>(formatter);
        } catch (const std::invalid_argument& ex) {
            return ex.what();
        }

        return {};
    }
}

// DEFAULT PATTERN TESTS ---------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("the default pattern layout") {
    const krumb::event ev = make_event(krumb::level::info, "hello");
    krumb::pattern_formatter formatter;
    std::string line;

    formatter.format(ev, line);

    CHECK(line == std::format("[2026-10-29 12:34:56.789] [info] [test_pattern_formatter.cpp:{}] hello", ev.location.line()));
}

TEST_CASE("the default pattern is exposed as a constant") {
    CHECK(krumb::pattern_formatter::default_pattern == "[{time:%F %T}] [{level}] [{file}:{line}] {message}");
}

TEST_CASE("no trailing newline is added") {
    krumb::pattern_formatter formatter;
    std::string line;

    formatter.format(make_event(krumb::level::info, "hello"), line);

    CHECK(line.back() != '\n');
}

TEST_CASE("output is appended to the buffer") {
    krumb::pattern_formatter formatter;
    std::string line = "prefix|";

    formatter.format(make_event(krumb::level::info, "hello"), line);

    CHECK(line.starts_with("prefix|[2026-10-29"));
}

// FIELD TESTS -------------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("level fields") {
    CHECK(render("{level}|{LEVEL}", make_event(krumb::level::warn, "x")) == "warn|WARN");
    CHECK(render("{level}|{LEVEL}", make_event(krumb::level::critical, "x")) == "critical|CRITICAL");
}

TEST_CASE("message field") {
    CHECK(render("{message}", make_event(krumb::level::info, "hello world")) == "hello world");
}

TEST_CASE("file field has no directory part, path field keeps it") {
    const krumb::event ev = make_event(krumb::level::info, "x");

    CHECK(render("{file}", ev) == "test_pattern_formatter.cpp");
    CHECK(render("{path}", ev) == ev.location.file_name());
}

TEST_CASE("line and function fields") {
    const krumb::event ev = make_event(krumb::level::info, "x");

    CHECK(render("{line}", ev) == std::to_string(ev.location.line()));
    CHECK(render("{function}", ev) == ev.location.function_name());
}

TEST_CASE("time field and its specs") {
    const krumb::event ev = make_event(krumb::level::info, "x");

    CHECK(render("{time}", ev) == "2026-10-29 12:34:56.789");
    CHECK(render("{time:%F}", ev) == "2026-10-29");
    CHECK(render("{time:%H:%M}", ev) == "12:34");
    CHECK(render("{time:%T}", ev) == "12:34:56.789");
    CHECK(render("{time:%Y}", ev) == "2026");
}

// SPEC TESTS --------------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("width, alignment and precision specs") {
    const krumb::event ev = make_event(krumb::level::info, "x");

    CHECK(render("{level:>8}", ev) == "    info");
    CHECK(render("{LEVEL:<6}|", ev) == "INFO  |");
    CHECK(render("{level:^8}|", ev) == "  info  |");
    CHECK(render("{file:.4}", ev) == "test");
    CHECK(render("{line:05}", ev) == std::format("{:05}", ev.location.line()));
}

// LITERAL TESTS -----------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("literal text is copied") {
    CHECK(render("plain text", make_event(krumb::level::info, "x")) == "plain text");
}

TEST_CASE("an empty pattern is valid and formats nothing") {
    CHECK(rejection("").empty());
    CHECK(render("", make_event(krumb::level::info, "x")).empty());
}

TEST_CASE("doubled braces are literal braces") {
    const krumb::event ev = make_event(krumb::level::info, "hi");

    CHECK(render("{{message}}", ev) == "{message}");
    CHECK(render("{{{message}}}", ev) == "{hi}");
}

TEST_CASE("fields and text can be mixed freely") {
    CHECK(render("<{LEVEL}> {message} (again: {message})", make_event(krumb::level::error, "boom")) == "<ERROR> boom (again: boom)");
}

// INVALID PATTERN TESTS ---------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("an unknown field is rejected") {
    CHECK(rejection("{foo}").find("unknown field 'foo'") != std::string::npos);
    CHECK_FALSE(rejection("{}").empty());
    CHECK_FALSE(rejection("{ level }").empty());
}

TEST_CASE("unbalanced braces are rejected") {
    CHECK(rejection("{level").find("closing") != std::string::npos);
    CHECK(rejection("oops }").find("opening") != std::string::npos);
}

TEST_CASE("a spec the field cannot use is rejected") {
    CHECK_FALSE(rejection("{level:d}").empty());
    CHECK_FALSE(rejection("{line:s}").empty());
}

TEST_CASE("a nested brace in a spec is rejected") {
    CHECK_FALSE(rejection("{level:{}}").empty());
}

TEST_CASE("an invalid pattern throws std::invalid_argument") {
    CHECK_THROWS_AS(krumb::pattern_formatter{"{foo}"}, std::invalid_argument);
}
