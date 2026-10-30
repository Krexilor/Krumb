// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <chrono>
#include <cstdio>
#include <format>
#include <memory>
#include <utility>
#include <stdexcept>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/logger.hpp>
#include <krumb/core/event.hpp>
#include <krumb/sinks/console_sink.hpp>

// LOGGER ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger::logger() : logger(std::make_shared<console_sink>()) {}

    logger::logger(std::shared_ptr<sink> target) : sink_(std::move(target)) {
        if (!sink_) {
            throw std::invalid_argument("krumb: logger requires a non-null sink");
        }
    }

    logger::~logger() {
        flush();
    }

    void logger::flush() noexcept {
        try {
            sink_->flush();
        }
        catch (const std::exception& ex) {
            report_error("flush failed", ex.what());
        }
        catch (...) {
            report_error("flush failed", "unknown exception");
        }
    }

    void logger::set_error_handler(error_handler handler) {
        std::lock_guard lock(handler_mutex_);
        handler_ = std::move(handler);
    }

    void logger::write(level lvl, const std::source_location& location, std::string_view message) noexcept {
        try {
            const event ev{
                .lvl = lvl,
                .timestamp = std::chrono::system_clock::now(),
                .location = location,
                .message = message
            };

            sink_->write(ev);

            if (lvl >= get_flush_level()) {
                sink_->flush();
            }
        }
        catch (const std::exception& ex) {
            report_error("sink failed", ex.what());
        }
        catch (...) {
            report_error("sink failed", "unknown exception");
        }
    }

    void logger::report_error(std::string_view context, std::string_view detail) noexcept {
        try {
            error_handler handler;

            {
                std::lock_guard lock(handler_mutex_);
                handler = handler_;
            }

            const std::string description = std::format("{}: {}", context, detail);

            if (handler) {
                handler(description);
            }
            else {
                std::fprintf(stderr, "krumb: %s\n", description.c_str());
            }
        }
        catch (...) {
            // The error path must never throw, so a failing handler is ignored.
        }
    }
}

// DEFAULT LOGGER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger& default_logger() {
        static logger instance;

        return instance;
    }

    void set_level(level lvl) noexcept {
        default_logger().set_level(lvl);
    }

    level get_level() noexcept {
        return default_logger().get_level();
    }

    void flush() noexcept {
        default_logger().flush();
    }
}
