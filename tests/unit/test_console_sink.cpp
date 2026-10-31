// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cstdio>
#include <memory>
#include <string>
#include <stdexcept>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

#include "helpers.hpp"

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // A plain format, so the expected output of each test is exact.
    std::unique_ptr<krumb::formatter> plain_format() {
        return std::make_unique<krumb::pattern_formatter>("{level}: {message}");
    }

    void log_every_level(std::FILE* stream, krumb::color_mode color) {
        krumb::logger logger = krumb::logger_builder()
            .min_level(krumb::level::trace)
            .add_sink(std::make_shared<krumb::console_sink>(plain_format(), krumb::console_options{stream, color}))
            .build();

        logger.log(krumb::level::trace, "m");
        logger.log(krumb::level::debug, "m");
        logger.log(krumb::level::info, "m");
        logger.log(krumb::level::warn, "m");
        logger.log(krumb::level::error, "m");
        logger.log(krumb::level::critical, "m");
    }

    constexpr const char* plain_output =
        "trace: m\n"
        "debug: m\n"
        "info: m\n"
        "warn: m\n"
        "error: m\n"
        "critical: m\n";
}

// CONSOLE SINK TESTS ------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("color mode always wraps each line in the color of its level") {
    test::temp_file file{"krumb_test_color_always.log"};
    test::stream_file stream{file.path()};

    log_every_level(stream.get(), krumb::color_mode::always);

    const std::string expected =
        "\x1b[90mtrace: m\x1b[0m\n"
        "\x1b[36mdebug: m\x1b[0m\n"
        "\x1b[32minfo: m\x1b[0m\n"
        "\x1b[33mwarn: m\x1b[0m\n"
        "\x1b[31merror: m\x1b[0m\n"
        "\x1b[1;31mcritical: m\x1b[0m\n";

    CHECK(test::read_file(file.path()) == expected);
}

TEST_CASE("color mode never writes plain lines") {
    test::temp_file file{"krumb_test_color_never.log"};
    test::stream_file stream{file.path()};

    log_every_level(stream.get(), krumb::color_mode::never);

    CHECK(test::read_file(file.path()) == plain_output);
}

TEST_CASE("color mode automatic writes plain lines when the stream is not a terminal") {
    test::temp_file file{"krumb_test_color_automatic.log"};
    test::stream_file stream{file.path()};

    log_every_level(stream.get(), krumb::color_mode::automatic);

    CHECK(test::read_file(file.path()) == plain_output);
}

TEST_CASE("the default console options use stdout with automatic color") {
    const krumb::console_options options;

    CHECK(options.stream == stdout);
    CHECK(options.color == krumb::color_mode::automatic);
}

TEST_CASE("a console sink rejects a null stream") {
    CHECK_THROWS_AS(krumb::console_sink(krumb::console_options{nullptr, krumb::color_mode::never}), std::invalid_argument);
}

TEST_CASE("a console sink rejects a null formatter") {
    CHECK_THROWS_AS(krumb::console_sink(std::unique_ptr<krumb::formatter>{}), std::invalid_argument);
}
