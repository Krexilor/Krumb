// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <exception>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

// EXAMPLE -----------------------------------------------------------------------------------------------------------------------------------------|
// The simple features of Krumb in one file, from the first log line to threads.
int main() {
    // 1. zero setup: include the header and log.
    krumb::info("Hello, {}!", "krumb");

    // 2. levels: the default threshold is info, so trace and debug are dropped.
    krumb::trace("dropped: below the default level");
    krumb::debug("dropped: below the default level");
    krumb::info("an informational message");
    krumb::warn("something looks suspicious");
    krumb::error("an operation failed");
    krumb::critical("the application cannot continue");

    // 3. formatting: messages use std::format syntax, checked at compile time.
    krumb::info("{} + {} = {}", 2, 3, 2 + 3);
    krumb::info("pi is about {:.3f}", 3.14159265);
    krumb::info("[{:>8}] [{:<8}] [{:^8}]", "right", "left", "mid");

    // 4. runtime level change: lower the threshold to see everything, then raise it.
    krumb::set_level(krumb::level::trace);
    krumb::trace("now visible: threshold is trace");

    krumb::set_level(krumb::level::warn);
    krumb::info("dropped: threshold is warn");
    krumb::warn("still visible: threshold is warn");

    krumb::set_level(krumb::level::info);

    // 5. explicit level: log() takes the level as an argument, for when it is only known at runtime.
    const krumb::level chosen = krumb::level::warn;
    krumb::log(chosen, "level chosen at runtime: {}", krumb::to_string(chosen));

    // 6. separate logger: an instance has its own threshold, independent of the default logger.
    krumb::logger audit;
    audit.set_level(krumb::level::error);
    audit.log(krumb::level::info, "dropped: below this logger's threshold");
    audit.log(krumb::level::error, "audit logger: write failed for user {}", 42);

    // 7. threads: each line is written as a whole, so output from different threads does not mix.
    std::vector<std::thread> workers;

    for (int id = 0; id < 3; id++) {
        workers.emplace_back([id] {
            for (int step = 0; step < 3; step++) {
                krumb::info("worker {} step {}", id, step);
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    // 8. file logging: a logger can write to a file instead of the console.
    try {
        auto file = std::make_shared<krumb::file_sink>("krumb_example.log", krumb::file_sink::mode::truncate);
        krumb::logger file_logger{file};

        file_logger.set_error_handler([](std::string_view description) {
            std::cerr << "logging problem: " << description << "\n";
        });

        file_logger.log(krumb::level::info, "written to the file only");
        file_logger.log(krumb::level::error, "an error is on disk right away");
        file_logger.flush();

        krumb::info("file logging done, see krumb_example.log");
    } 
    catch (const std::exception& ex) {
        krumb::error("could not set up file logging: {}", ex.what());
    }

    return 0;
}
