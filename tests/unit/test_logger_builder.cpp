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
    // Counts how many times it gets formatted, to prove that messages nobody wants are never formatted.
    struct counted {
        int* calls;
    };
}

template <>
struct std::formatter<counted> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const counted& value, std::format_context& ctx) const {
        ++*value.calls;

        return std::format_to(ctx.out(), "counted");
    }
};

// BUILDER TESTS -----------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("builder defaults") {
    krumb::logger logger = krumb::logger_builder().build();

    CHECK(logger.get_level() == krumb::level::info);
    CHECK(logger.get_flush_level() == krumb::level::error);
    CHECK(logger.should_log(krumb::level::info));
    CHECK_FALSE(logger.should_log(krumb::level::debug));
}

TEST_CASE("builder settings are applied") {
    krumb::logger logger = krumb::logger_builder()
        .min_level(krumb::level::debug)
        .flush_level(krumb::level::warn)
        .build();

    CHECK(logger.get_level() == krumb::level::debug);
    CHECK(logger.get_flush_level() == krumb::level::warn);
}

TEST_CASE("builder rejects a null sink") {
    krumb::logger_builder builder;

    CHECK_THROWS_AS(builder.add_sink(nullptr), std::invalid_argument);
}

TEST_CASE("each build creates an independent logger") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger_builder builder;

    builder.min_level(krumb::level::trace).add_sink(sink);

    krumb::logger first = builder.build();
    krumb::logger second = builder.build();

    first.set_level(krumb::level::error);

    CHECK(second.get_level() == krumb::level::trace);

    first.log(krumb::level::info, "dropped");
    second.log(krumb::level::info, "kept");

    CHECK(sink->messages() == std::vector<std::string>{"kept"});
}

TEST_CASE("builder error handler is used") {
    std::vector<std::string> errors;
    krumb::logger logger = krumb::logger_builder()
        .add_sink(std::make_shared<test::failing_sink>())
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    logger.log(krumb::level::info, "this write fails");

    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 write failed: simulated write error");
}

// MULTIPLE SINK TESTS -----------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("every sink receives a message") {
    auto first = std::make_shared<test::recording_sink>();
    auto second = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder().add_sink(first).add_sink(second).build();

    logger.log(krumb::level::info, "hello");

    CHECK(first->messages() == std::vector<std::string>{"hello"});
    CHECK(second->messages() == std::vector<std::string>{"hello"});
}

TEST_CASE("sink thresholds are independent") {
    auto verbose = std::make_shared<test::recording_sink>();
    auto quiet = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .min_level(krumb::level::trace)
        .add_sink(verbose, krumb::level::trace)
        .add_sink(quiet, krumb::level::warn)
        .build();

    logger.log(krumb::level::info, "a");
    logger.log(krumb::level::warn, "b");
    logger.log(krumb::level::error, "c");

    CHECK(verbose->messages() == std::vector<std::string>{"a", "b", "c"});
    CHECK(quiet->messages() == std::vector<std::string>{"b", "c"});
}

TEST_CASE("the logger level applies before the sink thresholds") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .min_level(krumb::level::warn)
        .add_sink(sink, krumb::level::trace)
        .build();

    logger.log(krumb::level::info, "dropped");
    logger.log(krumb::level::warn, "kept");

    CHECK(sink->messages() == std::vector<std::string>{"kept"});
}

TEST_CASE("should_log follows the lowest sink threshold") {
    krumb::logger logger = krumb::logger_builder()
        .min_level(krumb::level::trace)
        .add_sink(std::make_shared<test::recording_sink>(), krumb::level::warn)
        .add_sink(std::make_shared<test::recording_sink>(), krumb::level::error)
        .build();

    CHECK_FALSE(logger.should_log(krumb::level::info));
    CHECK(logger.should_log(krumb::level::warn));
    CHECK(logger.should_log(krumb::level::error));
}

TEST_CASE("a message that no sink wants is not formatted") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .min_level(krumb::level::trace)
        .add_sink(sink, krumb::level::error)
        .build();
    int calls = 0;

    logger.log(krumb::level::info, "{}", counted{&calls});
    CHECK(calls == 0);

    logger.log(krumb::level::error, "{}", counted{&calls});
    CHECK(calls == 1);
}

TEST_CASE("only the sinks that received a message are flushed") {
    auto everything = std::make_shared<test::recording_sink>();
    auto errors_only = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .flush_level(krumb::level::warn)
        .add_sink(everything, krumb::level::trace)
        .add_sink(errors_only, krumb::level::error)
        .build();

    logger.log(krumb::level::warn, "reaches only the first sink");

    CHECK(everything->flush_count() == 1);
    CHECK(errors_only->flush_count() == 0);

    logger.log(krumb::level::error, "reaches both");

    CHECK(everything->flush_count() == 2);
    CHECK(errors_only->flush_count() == 1);
}

TEST_CASE("flush reaches every sink") {
    auto first = std::make_shared<test::recording_sink>();
    auto second = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder().add_sink(first).add_sink(second).build();

    logger.flush();

    CHECK(first->flush_count() == 1);
    CHECK(second->flush_count() == 1);
}

// FAILURE ISOLATION TESTS -------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("a failing sink does not stop the other sinks") {
    std::vector<std::string> errors;
    auto good = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .add_sink(std::make_shared<test::failing_sink>())
        .add_sink(good)
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    CHECK_NOTHROW(logger.log(krumb::level::info, "hello"));

    CHECK(good->messages() == std::vector<std::string>{"hello"});
    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 write failed: simulated write error");
}

TEST_CASE("a failing sink is identified by its position") {
    std::vector<std::string> errors;
    auto good = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .add_sink(good)
        .add_sink(std::make_shared<test::failing_sink>())
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    logger.log(krumb::level::info, "hello");

    CHECK(good->messages() == std::vector<std::string>{"hello"});
    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 1 write failed: simulated write error");
}

TEST_CASE("a failed write is not followed by a flush of the same sink") {
    std::vector<std::string> errors;
    auto good = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .add_sink(std::make_shared<test::failing_sink>())
        .add_sink(good)
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    logger.log(krumb::level::error, "hello");

    CHECK(errors.size() == 1);
    CHECK(good->flush_count() == 1);
}

TEST_CASE("a failing flush does not stop the other sinks from flushing") {
    std::vector<std::string> errors;
    auto good = std::make_shared<test::recording_sink>();
    krumb::logger logger = krumb::logger_builder()
        .add_sink(std::make_shared<test::failing_sink>())
        .add_sink(good)
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    logger.flush();

    CHECK(good->flush_count() == 1);
    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 flush failed: simulated flush error");
}

// SHARED SINK TESTS -------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("one sink can be shared by loggers with different thresholds") {
    auto shared = std::make_shared<test::recording_sink>();
    krumb::logger strict = krumb::logger_builder()
        .min_level(krumb::level::trace)
        .add_sink(shared, krumb::level::warn)
        .build();
    krumb::logger relaxed = krumb::logger_builder()
        .min_level(krumb::level::trace)
        .add_sink(shared, krumb::level::trace)
        .build();

    strict.log(krumb::level::info, "dropped by the strict logger");
    relaxed.log(krumb::level::info, "from relaxed");
    strict.log(krumb::level::warn, "from strict");

    CHECK(shared->messages() == std::vector<std::string>{"from relaxed", "from strict"});
}

// HANDLE TESTS ------------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("a copy shares the level") {
    krumb::logger original;
    krumb::logger copy = original;

    copy.set_level(krumb::level::error);

    CHECK(original.get_level() == krumb::level::error);
}

TEST_CASE("a copy shares the sinks") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger original{sink};
    krumb::logger copy = original;

    copy.log(krumb::level::info, "through the copy");

    CHECK(sink->messages() == std::vector<std::string>{"through the copy"});
}

TEST_CASE("a copy shares the error handler") {
    std::vector<std::string> errors;
    krumb::logger original{std::make_shared<test::failing_sink>()};
    krumb::logger copy = original;

    copy.set_error_handler([&errors](std::string_view description) {
        errors.emplace_back(description);
    });

    original.log(krumb::level::info, "this write fails");

    CHECK(errors.size() == 1);
}

TEST_CASE("the sinks are flushed when the last handle goes away") {
    auto sink = std::make_shared<test::recording_sink>();

    {
        krumb::logger first{sink};

        {
            krumb::logger second = first;
        }

        CHECK(sink->flush_count() == 0);
    }

    CHECK(sink->flush_count() == 1);
}

TEST_CASE("assigning a logger replaces its state and flushes the old sinks") {
    auto old_sink = std::make_shared<test::recording_sink>();
    auto new_sink = std::make_shared<test::recording_sink>();
    krumb::logger target{old_sink};
    krumb::logger source{new_sink};

    target = source;

    CHECK(old_sink->flush_count() == 1);

    target.log(krumb::level::info, "after assignment");

    CHECK(new_sink->messages() == std::vector<std::string>{"after assignment"});
    CHECK(old_sink->messages().empty());
}

// DEFAULT LOGGER TESTS ----------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("set_default_logger routes the free functions") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger custom = krumb::logger_builder().min_level(krumb::level::trace).add_sink(sink).build();
    const krumb::logger previous = krumb::default_logger();

    krumb::set_default_logger(custom);

    krumb::debug("debug {}", 1);
    krumb::error("error {}", 2);
    krumb::flush();

    krumb::set_default_logger(previous);

    CHECK(sink->messages() == std::vector<std::string>{"debug 1", "error 2"});
    CHECK(sink->levels() == std::vector<krumb::level>{krumb::level::debug, krumb::level::error});

    CHECK(sink->flush_count() == 2);
}

TEST_CASE("an earlier handle keeps pointing at the old default logger") {
    const krumb::logger previous = krumb::default_logger();
    const krumb::level previous_level = previous.get_level();
    krumb::logger custom = krumb::logger_builder().min_level(krumb::level::trace).build();

    krumb::set_default_logger(custom);
    krumb::set_level(krumb::level::error);

    CHECK(custom.get_level() == krumb::level::error);
    CHECK(previous.get_level() == previous_level);

    krumb::set_default_logger(previous);

    CHECK(krumb::get_level() == previous_level);
}
