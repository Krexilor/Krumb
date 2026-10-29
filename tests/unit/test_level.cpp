// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/level.hpp>

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// LEVEL TESTS -------------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("level names") {
    CHECK(krumb::to_string(krumb::level::trace) == "trace");
    CHECK(krumb::to_string(krumb::level::debug) == "debug");
    CHECK(krumb::to_string(krumb::level::info) == "info");
    CHECK(krumb::to_string(krumb::level::warn) == "warn");
    CHECK(krumb::to_string(krumb::level::error) == "error");
    CHECK(krumb::to_string(krumb::level::critical) == "critical");
    CHECK(krumb::to_string(krumb::level::off) == "off");
}

TEST_CASE("levels are ordered by severity") {
    CHECK(krumb::level::trace < krumb::level::debug);
    CHECK(krumb::level::debug < krumb::level::info);
    CHECK(krumb::level::info < krumb::level::warn);
    CHECK(krumb::level::warn < krumb::level::error);
    CHECK(krumb::level::error < krumb::level::critical);
    CHECK(krumb::level::critical < krumb::level::off);
}

TEST_CASE("out-of-range level has a fallback name") {
    CHECK(krumb::to_string(static_cast<krumb::level>(255)) == "unknown");
}
