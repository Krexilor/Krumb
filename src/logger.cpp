// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <chrono>
#include <cstdio>
#include <format>
#include <memory>
#include <utility>
#include <exception>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/logger.hpp>
#include <krumb/logger_builder.hpp>

#include <krumb/core/event.hpp>
#include <krumb/sinks/console_sink.hpp>

// LOGGER STATE ------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb::detail {
    logger_state::~logger_state() {
        flush();
    }

    void logger_state::write(level lvl, const std::source_location& location, std::string_view message) noexcept {
        const event ev{
            .lvl = lvl,
            .timestamp = std::chrono::system_clock::now(),
            .location = location,
            .message = message
        };

        const level flush_at = flush_level.load(std::memory_order_relaxed);

        for (std::size_t index = 0; index < sinks.size(); index++) {
            const sink_entry& entry = sinks[index];

            if (lvl < entry.threshold) {
                continue;
            }

            try {
                entry.target->write(ev);
            }
            catch (const std::exception& ex) {
                report_sink_error(index, "write", ex.what());
                continue;
            }
            catch (...) {
                report_sink_error(index, "write", "unknown exception");
                continue;
            }

            if (lvl >= flush_at) {
                flush_sink(index);
            }
        }
    }

    void logger_state::flush() noexcept {
        for (std::size_t index = 0; index < sinks.size(); index++) {
            flush_sink(index);
        }
    }

    void logger_state::flush_sink(std::size_t index) noexcept {
        try {
            sinks[index].target->flush();
        }
        catch (const std::exception& ex) {
            report_sink_error(index, "flush", ex.what());
        }
        catch (...) {
            report_sink_error(index, "flush", "unknown exception");
        }
    }

    void logger_state::report_error(std::string_view context, std::string_view detail) noexcept {
        try {
            deliver(std::format("{}: {}", context, detail));
        }
        catch (...) {
            // The error path must never throw, so a failiing handler is ignored.
        }
    }

    void logger_state::report_sink_error(std::size_t index, std::string_view operation, std::string_view detail) noexcept {
        try {
            deliver(std::format("sink {} {} failed: {}", index, operation, detail));
        }
        catch (...) {
            // The error path must never throw, so a failing handler is ignored.
        }
    }

    void logger_state::deliver(std::string_view description) {
        error_handler current;

        {
            std::lock_guard lock(handler_mutex);
            current = handler;
        }

        if (current) {
            current(description);
        }
        else {
            std::fprintf(stderr, "krumb: %.*s\n", static_cast<int>(description.size()), description.data());
        }
    }
}

// LOGGER ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger::logger() : logger(std::make_shared<console_sink>()) {}
    logger::logger(std::shared_ptr<sink> target) : logger(logger_builder().add_sink(std::move(target)).build()) {}
    logger::logger(std::shared_ptr<detail::logger_state> state) noexcept : state_(std::move(state)) {}

    void logger::set_error_handler(error_handler handler) {
        std::lock_guard lock(state_->handler_mutex);
        state_->handler = std::move(handler);
    }
}

// DEFAULT LOGGER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    struct default_holder {
        std::mutex mutex;
        krumb::logger current;
    };

    default_holder& holder() {
        static default_holder instance;

        return instance;
    }
}

namespace krumb {
    logger default_logger() {
        default_holder& held = holder();
        std::lock_guard lock(held.mutex);

        return held.current;
    }

    void set_default_logger(logger replacement) {
        default_holder& held = holder();

        {
            std::lock_guard lock(held.mutex);
            std::swap(held.current, replacement);
        }

        // `replacement` now holds the previous default logger. It is destroyed here,
        // because destroying the last handle flushes its sinks and may call an error handler.
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
