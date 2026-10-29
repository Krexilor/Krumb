// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <format>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    struct probe {
        int* calls;
    };
}

template <>
struct std::formatter<probe> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const probe& value, std::format_context& ctx) const {
        ++*value.calls;

        return std::format_to(ctx.out(), "probe");
    }
};

// LOGGER TESTS ------------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("default threshold is info") {
    krumb::logger logger;

    CHECK(logger.get_level() == krumb::level::info);
    CHECK_FALSE(logger.should_log(krumb::level::debug));
    CHECK(logger.should_log(krumb::level::info));
}

TEST_CASE("threshold boundary") {
    krumb::logger logger;
    logger.set_level(krumb::level::warn);

    CHECK_FALSE(logger.should_log(krumb::level::trace));
    CHECK_FALSE(logger.should_log(krumb::level::debug));
    CHECK_FALSE(logger.should_log(krumb::level::info));
    CHECK(logger.should_log(krumb::level::warn));
    CHECK(logger.should_log(krumb::level::error));
    CHECK(logger.should_log(krumb::level::critical));
}

TEST_CASE("runtime level change takes effect") {
    krumb::logger logger;

    logger.set_level(krumb::level::error);
    CHECK_FALSE(logger.should_log(krumb::level::warn));

    logger.set_level(krumb::level::trace);
    CHECK(logger.should_log(krumb::level::trace));
}

TEST_CASE("off disables everything and is never a message level") {
    krumb::logger logger;
    logger.set_level(krumb::level::off);

    CHECK_FALSE(logger.should_log(krumb::level::critical));

    logger.set_level(krumb::level::trace);
    CHECK_FALSE(logger.should_log(krumb::level::off));
}

TEST_CASE("filtered messages are not formatted") {
    krumb::logger logger;
    int calls = 0;

    logger.log(krumb::level::debug, "{}", probe{&calls});

    CHECK(calls == 0);
}

TEST_CASE("free functions control the default logger") {
    const krumb::level previous = krumb::get_level();

    CHECK(&krumb::default_logger() == &krumb::default_logger());

    krumb::set_level(krumb::level::error);

    CHECK(krumb::get_level() == krumb::level::error);
    CHECK(krumb::default_logger().get_level() == krumb::level::error);
    CHECK_FALSE(krumb::default_logger().should_log(krumb::level::info));

    krumb::set_level(previous);
}
