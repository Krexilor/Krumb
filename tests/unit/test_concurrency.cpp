// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <set>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <cstddef>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

#include "helpers.hpp"

// EXTERNAL HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include "third_party/doctest.h"

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    constexpr int thread_count = 8;
    constexpr int messages_per_thread = 500;
    constexpr std::size_t total_messages = static_cast<std::size_t>(thread_count) * messages_per_thread;

    // Runs work(thread_index) on `count` threads and waits for all of them.
    template <typename Work>
    void run_threads(int count, Work work) {
        std::vector<std::thread> threads;

        for (int index = 0; index < count; index++) {
            threads.emplace_back(work, index);
        }

        for (std::thread& thread : threads) {
            thread.join();
        }
    }
}

// CONCURRENCY TESTS -------------------------------------------------------------------------------------------------------------------------------|
TEST_CASE("every message from many threads reaches the sink exactly once") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};

    run_threads(thread_count, [&logger](int id) {
        for (int n = 0; n < messages_per_thread; n++) {
            logger.log(krumb::level::info, "thread {} message {}", id, n);
        }
    });

    const auto messages = sink->messages();
    const std::set<std::string> unique(messages.begin(), messages.end());

    CHECK(messages.size() == total_messages);
    CHECK(unique.size() == total_messages);
}

TEST_CASE("lines stay whole in a file shared by two loggers under contention") {
    test::temp_file file{"krumb_test_contention.log"};

    {
        auto sink = std::make_shared<krumb::file_sink>(file.path());
        krumb::logger first{sink};
        krumb::logger second{sink};

        run_threads(thread_count, [&](int id) {
            krumb::logger& target = (id % 2 == 0) ? first : second;

            for (int n = 0; n < messages_per_thread; n++) {
                target.log(krumb::level::info, "t{} m{} payload-ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789-end", id, n);
            }
        });
    }

    const auto lines = test::split_lines(test::read_file(file.path()));
    std::size_t whole = 0;

    for (const std::string& line : lines) {
        if (line.starts_with('[') && line.ends_with(" payload-ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789-end")) {
            whole++;
        }
    }

    CHECK(lines.size() == total_messages);
    CHECK(whole == total_messages);
}

TEST_CASE("changing the level while other threads log is safe") {
    auto sink = std::make_shared<test::recording_sink>();
    krumb::logger logger{sink};
    std::atomic<bool> done{false};

    std::thread toggler([&] {
        while (!done.load()) {
            logger.set_level(krumb::level::error);
            logger.set_level(krumb::level::info);
            std::this_thread::yield();
        }
    });

    run_threads(thread_count, [&logger](int id) {
        for (int n = 0; n < messages_per_thread; n++) {
            logger.log(krumb::level::info, "thread {} message {}", id, n);
        }
    });

    done.store(true);
    toggler.join();

    // How many messages got through depends on timing. The point of this test is that nothing crashes or races.
    CHECK(sink->messages().size() <= total_messages);
}

TEST_CASE("every sink failure under contention is reported") {
    std::atomic<int> reports{0};
    krumb::logger logger{std::make_shared<test::failing_sink>()};

    logger.set_error_handler([&reports](std::string_view) {
        reports.fetch_add(1);
    });

    run_threads(thread_count, [&logger](int id) {
        for (int n = 0; n < messages_per_thread; n++) {
            logger.log(krumb::level::info, "thread {} message {}", id, n);
        }
    });

    CHECK(reports.load() == static_cast<int>(total_messages));
}

TEST_CASE("the error handler can be replaced while threads log") {
    krumb::logger logger{std::make_shared<test::failing_sink>()};
    std::atomic<bool> done{false};

    std::thread replacer([&] {
        while (!done.load()) {
            logger.set_error_handler([](std::string_view) {});
            std::this_thread::yield();
        }
    });

    run_threads(thread_count, [&logger](int id) {
        for (int n = 0; n < messages_per_thread; n++) {
            logger.log(krumb::level::info, "thread {} message {}", id, n);
        }
    });

    done.store(true);
    replacer.join();

    // No assertion on counts: the handler changes constantly. The point is that nothing crashes or races.
    CHECK(true);
}
