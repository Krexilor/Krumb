// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <memory>
#include <string>
#include <filesystem>
#include <system_error>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

#include "helpers.hpp"

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// FILE SINK TESTS ---------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("file sink writes one line per message") {
    test::temp_file file{"krumb_test_lines.log"};

    {
        auto sink = std::make_shared<krumb::file_sink>(file.path());
        krumb::logger logger{sink};

        logger.log(krumb::level::info, "first {}", 1);
        logger.log(krumb::level::warn, "second");
    }

    const auto lines = test::split_lines(test::read_file(file.path()));

    REQUIRE(lines.size() == 2);
    CHECK(lines[0].find("[info]") != std::string::npos);
    CHECK(lines[0].ends_with("first 1"));
    CHECK(lines[1].find("[warn]") != std::string::npos);
    CHECK(lines[1].ends_with("second"));
}

TEST_CASE("file sink appends by default") {
    test::temp_file file{"krumb_test_append.log"};

    for (int run = 0; run < 2; run++) {
        auto sink = std::make_shared<krumb::file_sink>(file.path());
        krumb::logger logger{sink};

        logger.log(krumb::level::info, "run {}", run);
    }

    CHECK(test::split_lines(test::read_file(file.path())).size() == 2);
}

TEST_CASE("file sink truncate mode starts a fresh file") {
    test::temp_file file{"krumb_test_truncate.log"};

    {
        auto sink = std::make_shared<krumb::file_sink>(file.path());
        krumb::logger logger{sink};

        logger.log(krumb::level::info, "old line");
    }

    {
        auto sink = std::make_shared<krumb::file_sink>(file.path(), krumb::file_sink::mode::truncate);
        krumb::logger logger{sink};

        logger.log(krumb::level::info, "after truncate");
    }

    const auto lines = test::split_lines(test::read_file(file.path()));

    REQUIRE(lines.size() == 1);
    CHECK(lines[0].ends_with("after truncate"));
}

TEST_CASE("file sink throws when the file cannot be opened") {
    const auto missing = std::filesystem::temp_directory_path() / "krumb_no_such_dir" / "file.log";

    CHECK_THROWS_AS(krumb::file_sink{missing}, std::system_error);
}

TEST_CASE("an error message is on disk before the file is closed") {
    test::temp_file file{"krumb_test_error_flush.log"};
    auto sink = std::make_shared<krumb::file_sink>(file.path());
    krumb::logger logger{sink};

    logger.log(krumb::level::info, "buffered");
    logger.log(krumb::level::error, "flushed");

    CHECK(test::split_lines(test::read_file(file.path())).size() == 2);
}

TEST_CASE("manual flush makes a message visible") {
    test::temp_file file{"krumb_test_manual_flush.log"};
    auto sink = std::make_shared<krumb::file_sink>(file.path());
    krumb::logger logger{sink};

    logger.log(krumb::level::info, "buffered");
    logger.flush();

    CHECK(test::split_lines(test::read_file(file.path())).size() == 1);
}

TEST_CASE("destroying a logger flushes a sink that is still open") {
    test::temp_file file{"krumb_test_logger_flush.log"};
    auto sink = std::make_shared<krumb::file_sink>(file.path());

    {
        krumb::logger logger{sink};

        logger.log(krumb::level::info, "buffered");
    }

    CHECK(test::split_lines(test::read_file(file.path())).size() == 1);
}

TEST_CASE("two loggers can share one file sink") {
    test::temp_file file{"krumb_test_shared.log"};

    {
        auto sink = std::make_shared<krumb::file_sink>(file.path());
        krumb::logger first{sink};
        krumb::logger second{sink};

        first.log(krumb::level::info, "from first");
        second.log(krumb::level::info, "from second");
    }

    CHECK(test::split_lines(test::read_file(file.path())).size() == 2);
}
