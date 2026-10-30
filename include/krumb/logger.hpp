// RELIABILITY CONTRACT ----------------------------------------------------------------------------------------------------------------------------|
// thread safety
//   - every logger member function can be called concurrently from any thread.
//   - the built-in sinks lock internally: a line is never interleaved with another line, even when several loggers share one sink.
//   - events reach a sink in the order they acquire its lock. timestamps are taken before that, so they can be slightly out of order.
//   - set_level and set_flush_level are atomic. a message that already passed the level check may still be written after the level is raised.
//   - a custom sink must be thread-safe itself. the error handler can be called from any thread, so it must be thread-safe too.
//   - destroying a logger while another thread is using it is a data race and is not supported.
//
// failures
//   - constructing a file_sink throws std::system_error if the file cannot be opened.
//   - while logging, nothing is thrown to the caller. a sink failure or a formatting failure goes to the error handler,
//     and the message is dropped. the default handler prints one line to stderr. exceptions thrown by a handler are ignored.
//   - the logger's destructor flushes the sink, so the error handler can still be called then.
//     anything the handler uses must stay valid until the logger is destroyed.
//
// flushing
//   - flush() flushes the sink now.
//   - after a message at or above the flush level (default: error) the sink is flushed automatically. `off` disables this.
//   - destroying a logger flushes its sink. destroying the last owner of a file_sink flushes and closes the file.
//   - lines below the flush level can be lost if the process crashes before they are flushed.
//
// shutdown
//   - the default logger is a function-local static and is destroyed during static destruction.
//     logging from static destructors is not supported.
//   - normal process exit flushes stdout, so the console sink needs no extra step.

#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <memory>
#include <atomic>
#include <format>
#include <string>
#include <utility>
#include <exception>
#include <functional>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/sink.hpp>
#include <krumb/core/level.hpp>
#include <krumb/core/format_string.hpp>

// LOGGER ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Formats messages, filters them by level, and hands them to a sink.
    // All member functions can be called concurrently. a logger cannot be copied or moved.
    class logger {
        public:
        using error_handler = std::function<void(std::string_view)>;

        logger();
        explicit logger(std::shared_ptr<sink> target);
        
        ~logger();

        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;

        void set_level(level lvl) noexcept {
            level_.store(lvl, std::memory_order_relaxed);
        }

        level get_level() const noexcept {
            return level_.load(std::memory_order_relaxed);
        }

        bool should_log(level lvl) const noexcept {
            return lvl != level::off && lvl >= get_level();
        }

        // After writing a message at or above this level, the sink is flushed. the default is `error`.
        // `off` disables automatic flushing.
        void set_flush_level(level lvl) noexcept {
            flush_level_.store(lvl, std::memory_order_relaxed);
        }

        level get_flush_level() const noexcept {
            return flush_level_.load(std::memory_order_relaxed);
        }

        void flush() noexcept;
        void set_error_handler(error_handler handler);

        template <typename... Args>
        void log(level lvl, location_format_string<Args...> fmt, Args&&... args) {
            if (!should_log(lvl)) {
                return;
            }

            std::string message;

            try {
                message = std::format(fmt.fmt, std::forward<Args>(args)...);
            }
            catch (const std::exception& ex) {
                report_error("message formatting failed", ex.what());
                return;
            }
            catch (...) {
                report_error("message formatting failed", "unknown exception");
                return;
            }

            write(lvl, fmt.location, message);
        }

        private:
        void write(level lvl, const std::source_location& location, std::string_view message) noexcept;

        void report_error(std::string_view context, std::string_view detail) noexcept;

        std::atomic<level> level_{level::info};
        std::atomic<level> flush_level_{level::error};
        std::shared_ptr<sink> sink_;

        std::mutex handler_mutex_;
        error_handler handler_;
    };
}

// DEFAULT LOGGER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger& default_logger();
    void set_level(level lvl) noexcept;
    level get_level() noexcept;
    void flush() noexcept;
}

// FREE FUNCTIONS ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    template <typename... Args>
    void log(level lvl, location_format_string<Args...> fmt, Args&&... args) {
        default_logger().log(lvl, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void trace(location_format_string<Args...> fmt, Args&&... args) {
        krumb::log(level::trace, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(location_format_string<Args...> fmt, Args&&... args) {
        krumb::log(level::debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(location_format_string<Args...> fmt, Args&&... args) {
        krumb::log(level::info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(location_format_string<Args...> fmt, Args&&... args) {
        krumb::log(level::warn, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(location_format_string<Args...> fmt, Args&&... args) {
        krumb::log(level::error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void critical(location_format_string<Args...> fmt, Args&&... args) {
        krumb::log(level::critical, fmt, std::forward<Args>(args)...);
    }
}
