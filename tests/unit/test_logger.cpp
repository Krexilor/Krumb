// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <format>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

#include "helpers.hpp"

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // counts how many times it gets formatted, to prove filtered messages are never formatted.
    struct probe {
        int* calls;
    };

    // always throws when formatted.
    struct explosive {};
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

template <>
struct std::formatter<explosive> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const explosive&, std::format_context&) const -> std::format_context::iterator {
        throw std::runtime_error("bad value");
    }
};

// FILTERING TESTS ---------------------------------------------------------------------------------------------------------------------------------|
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

    // a handle shares state with the default logger, so a change through the free functions is visible through it.
    krumb::logger handle = krumb::default_logger();

    krumb::set_level(krumb::level::error);

    CHECK(krumb::get_level() == krumb::level::error);
    CHECK(handle.get_level() == krumb::level::error);
    CHECK_FALSE(handle.should_log(krumb::level::info));

    krumb::set_level(previous);
}

// DELIVERY TESTS ----------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("messages reach the sink with their level") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};

    logger.log(krumb::level::warn, "value = {}", 42);

    CHECK(sink->messages() == std::vector<std::string>{"value = 42"});
    CHECK(sink->levels() == std::vector<krumb::level>{krumb::level::warn});
}

TEST_CASE("messages below the threshold do not reach the sink") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};
    logger.set_level(krumb::level::warn);

    logger.log(krumb::level::info, "dropped");
    logger.log(krumb::level::warn, "kept");

    CHECK(sink->messages() == std::vector<std::string>{"kept"});
}

TEST_CASE("a null sink is rejected") {
    CHECK_THROWS_AS(krumb::logger{std::shared_ptr<krumb::sink>{}}, std::invalid_argument);
}

// FLUSH TESTS -------------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("default flush level is error") {
    krumb::logger logger;

    CHECK(logger.get_flush_level() == krumb::level::error);
}

TEST_CASE("sink is flushed after messages at or above the flush level") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};

    logger.log(krumb::level::info, "no flush");
    logger.log(krumb::level::warn, "no flush");
    CHECK(sink->flush_count() == 0);

    logger.log(krumb::level::error, "flush");
    CHECK(sink->flush_count() == 1);

    logger.set_flush_level(krumb::level::warn);
    logger.log(krumb::level::warn, "flush");
    CHECK(sink->flush_count() == 2);

    logger.set_flush_level(krumb::level::off);
    logger.log(krumb::level::critical, "no flush");
    CHECK(sink->flush_count() == 2);
}

TEST_CASE("dropped messages do not flush") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};
    logger.set_level(krumb::level::warn);
    logger.set_flush_level(krumb::level::trace);

    logger.log(krumb::level::debug, "dropped");

    CHECK(sink->flush_count() == 0);
}

TEST_CASE("manual flush reaches the sink") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};

    logger.flush();

    CHECK(sink->flush_count() == 1);
}

TEST_CASE("destroying a logger flushes its sink") {
    auto sink = std::make_shared<test::recording_sink>();

    {
        krumb::logger logger{sink};
        logger.log(krumb::level::info, "buffered");
        CHECK(sink->flush_count() == 0);
    }

    CHECK(sink->flush_count() == 1);
}

// ERROR POLICY TESTS ------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("a failing write goes to the error handler and does not throw") {
    std::vector<std::string> errors;
    krumb::logger logger{std::make_shared<test::failing_sink>()};

    logger.set_error_handler([&errors](std::string_view description) {
        errors.emplace_back(description);
    });

    CHECK_NOTHROW(logger.log(krumb::level::error, "this write fails"));

    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 write failed: simulated write error");
}

TEST_CASE("a failing flush goes to the error handler and does not throw") {
    std::vector<std::string> errors;
    krumb::logger logger{std::make_shared<test::failing_sink>()};

    logger.set_error_handler([&errors](std::string_view description) {
        errors.emplace_back(description);
    });

    CHECK_NOTHROW(logger.flush());

    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 flush failed: simulated flush error");
}

TEST_CASE("a formatting failure goes to the error handler and drops the message") {
    std::vector<std::string> errors;
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};

    logger.set_error_handler([&errors](std::string_view description) {
        errors.emplace_back(description);
    });

    CHECK_NOTHROW(logger.log(krumb::level::info, "{}", explosive{}));

    CHECK(sink->messages().empty());
    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "message formatting failed: bad value");
}

TEST_CASE("an error handler that throws is ignored") {
    krumb::logger logger{std::make_shared<test::failing_sink>()};

    logger.set_error_handler([](std::string_view) {
        throw std::runtime_error("handler failed");
    });

    CHECK_NOTHROW(logger.log(krumb::level::info, "this write fails"));
}
