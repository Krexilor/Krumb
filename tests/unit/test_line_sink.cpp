// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // Wraps the message in angle brackets.
    class tag_formatter : public krumb::formatter {
        public:
        void format(const krumb::event& ev, std::string& out) override {
            out += '<';
            out += ev.message;
            out += '>';
        }
    };

    class throwing_formatter : public krumb::formatter {
        public:
        void format(const krumb::event&, std::string&) override {
            throw std::runtime_error("bad format");
        }
    };

    // A custom line sink: it only records what it is given.
    class collecting_sink : public krumb::line_sink {
        public:
        using line_sink::line_sink;

        std::vector<std::string> lines;
        std::vector<krumb::level> levels;
        int flushes = 0;

        private:
        void output(krumb::level lvl, std::string_view line) override {
            levels.push_back(lvl);
            lines.emplace_back(line);
        }

        void flush_output() override {
            flushes++;
        }
    };

    class failing_output_sink : public krumb::line_sink {
        private:
        void output(krumb::level, std::string_view) override {
            throw std::runtime_error("disk full");
        }

        void flush_output() override {
        }
    };
}

// LINE SINK TESTS ---------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("a line sink receives the formatted line with a trailing newline") {
    auto sink = std::make_shared<collecting_sink>(std::make_unique<tag_formatter>());
    krumb::logger logger{sink};

    logger.log(krumb::level::info, "hello {}", 1);

    CHECK(sink->lines == std::vector<std::string>{"<hello 1>\n"});
}

TEST_CASE("a line sink without a formatter uses the default pattern") {
    auto sink = std::make_shared<collecting_sink>();
    krumb::logger logger{sink};

    logger.log(krumb::level::warn, "hello");

    REQUIRE(sink->lines.size() == 1);
    CHECK(sink->lines[0].starts_with("["));
    CHECK(sink->lines[0].find("[warn]") != std::string::npos);
    CHECK(sink->lines[0].ends_with(" hello\n"));
}

TEST_CASE("the level is passed to output") {
    auto sink = std::make_shared<collecting_sink>(std::make_unique<tag_formatter>());
    krumb::logger logger = krumb::logger_builder().min_level(krumb::level::trace).add_sink(sink).build();

    logger.log(krumb::level::debug, "a");
    logger.log(krumb::level::error, "b");

    CHECK(sink->levels == std::vector<krumb::level>{krumb::level::debug, krumb::level::error});
}

TEST_CASE("a line sink rejects a null formatter") {
    CHECK_THROWS_AS(collecting_sink{std::unique_ptr<krumb::formatter>{}}, std::invalid_argument);
}

TEST_CASE("flushing the logger reaches flush_output") {
    auto sink = std::make_shared<collecting_sink>();
    krumb::logger logger{sink};

    logger.flush();

    CHECK(sink->flushes == 1);
}

TEST_CASE("a formatter that throws is reported as a sink failure") {
    std::vector<std::string> errors;
    auto sink = std::make_shared<collecting_sink>(std::make_unique<throwing_formatter>());
    krumb::logger logger = krumb::logger_builder()
        .add_sink(sink)
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    CHECK_NOTHROW(logger.log(krumb::level::info, "x"));

    CHECK(sink->lines.empty());
    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 write failed: bad format");
}

TEST_CASE("an output that throws is reported as a sink failure") {
    std::vector<std::string> errors;
    krumb::logger logger = krumb::logger_builder()
        .add_sink(std::make_shared<failing_output_sink>())
        .on_error([&errors](std::string_view description) {
            errors.emplace_back(description);
        })
        .build();

    CHECK_NOTHROW(logger.log(krumb::level::info, "x"));

    REQUIRE(errors.size() == 1);
    CHECK(errors[0] == "sink 0 write failed: disk full");
}

TEST_CASE("a custom line sink can be mixed with built-in sinks") {
    auto custom = std::make_shared<collecting_sink>(std::make_unique<tag_formatter>());
    auto raw = std::make_shared<collecting_sink>();
    krumb::logger logger = krumb::logger_builder().add_sink(custom).add_sink(raw, krumb::level::error).build();

    logger.log(krumb::level::info, "info message");
    logger.log(krumb::level::error, "error message");

    CHECK(custom->lines.size() == 2);
    CHECK(raw->lines.size() == 1);
}
