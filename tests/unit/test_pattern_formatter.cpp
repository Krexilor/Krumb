// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <chrono>
#include <format>
#include <string>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/event.hpp>
#include <krumb/formatters/text_formatter.hpp>

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    krumb::event make_event(krumb::level lvl, std::string_view message) {
        using namespace std::chrono_literals;

        return krumb::event{
            .lvl = lvl,
            .timestamp = std::chrono::sys_days{2026y / 10 / 29} + 12h + 34min + 56s + 789ms,
            .location = std::source_location::current(),
            .message = message
        };
    }
}

// TEXT FORMATTER TESTS ----------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("line layout") {
    const krumb::event ev = make_event(krumb::level::info, "hello");
    krumb::text_formatter formatter;
    std::string line;

    formatter.format(ev, line);

    const std::string tail = std::format(" [test_text_formatter.cpp:{}] hello", ev.location.line());

    CHECK(line.starts_with("[2026-10-29 12:34:56.789] [info]"));
    CHECK(line.ends_with(tail));
}

TEST_CASE("level name is used") {
    krumb::text_formatter formatter;
    std::string line;

    formatter.format(make_event(krumb::level::critical, "boom"), line);

    CHECK(line.find("[critical]") != std::string::npos);
}

TEST_CASE("file name has no directory part") {
    krumb::text_formatter formatter;
    std::string line;

    formatter.format(make_event(krumb::level::info, "hello"), line);

    CHECK(line.find('/') == std::string::npos);
    CHECK(line.find('\\') == std::string::npos);
}

TEST_CASE("no trailing newline is added") {
    krumb::text_formatter formatter;
    std::string line;

    formatter.format(make_event(krumb::level::info, "hello"), line);

    CHECK(line.back() != '\n');
}

TEST_CASE("output is appended to the buffer") {
    krumb::text_formatter formatter;
    std::string line = "prefix|";

    formatter.format(make_event(krumb::level::info, "hello"), line);

    CHECK(line.starts_with("prefix|[2026-10-29"));
}
