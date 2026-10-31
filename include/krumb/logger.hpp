// RELIABILITY CONTRACT ----------------------------------------------------------------------------------------------------------------------------|
// logger handles
//   - a logger is a handle to shared state. copying a logger gives another handle to the same level, sinks and error handler.
//   - when the last handle is destroyed, every sink is flushed.
//
// thread safety
//   - every member function can be called concurrently from any thread, including through different handles of the same logger.
//   - one logger object must not be assigned to while another thread uses that same object. give each thread its own handle.
//   - the sinks of a logger are fixed when it is built. the built-in sinks lock internally: a line is never interleaved with another line,
//     even when several loggers share one sink.
//   - events reach a sink in the order they acquire its lock. there is no ordering guarantee between different sinks.
//     timestamps are taken before that, so they can be slightly out of order.
//   - set_level and set_flush_level are atomic. a message that already passed the level check may still be written after the level is raised.
//   - a custom sink must be thread-safe itself. the error handler can be called from any thread, so it must be thread-safe too.
//
// filtering
//   - a message must pass the logger level. each sink then receives it only if it is at or above that sink's own threshold.
//   - a message below every sink's threshold is dropped before it is formatted.
//
// failures
//   - constructing a file_sink throws std::system_error if the file cannot be opened.
//   - while logging, nothing is thrown to the caller. each sink is tried independently, so one failing sink does not stop the others.
//   - a failure goes to the error handler as "sink <index> write failed: <reason>" or "sink <index> flush failed: <reason>",
//     where the index is the order in which sinks were added. a formatting failure is reported as "message formatting failed: <reason>"
//     and the message is dropped. the default handler prints one line to stderr. exceptions thrown by a handler are ignored.
//   - after a failed write, that sink is not flushed for the same message.
//
// custom sinks and formatters
//   - a class derived from line_sink needs no locking: output() and flush_output() are called one thread at a time.
//     anything else it exposes to other threads, such as a getter, needs its own lock.
//   - a formatter belongs to one sink and is called under that sink's lock, one thread at a time. do not share a formatter between sinks.
//   - a class derived directly from sink must be thread-safe itself.
//   - an invalid pattern throws std::invalid_argument when the pattern_formatter is constructed. logging never throws because of a pattern.
//   - the console sink decides about color once, when it is constructed.
//
// flushing
//   - flush() flushes every sink now.
//   - after a message at or above the flush level (default: error) the sinks that received it are flushed. `off` disables this.
//   - destroying the last handle of a logger flushes its sinks, so the error handler can still be called then.
//     anything the handler uses must stay valid until the last handle is destroyed.
//   - lines below the flush level can be lost if the process crashes before they are flushed.
//
// shutdown
//   - the default logger is held in a function-local static and is destroyed during static destruction.
//     logging from static destructors is not supported.
//   - normal process exit flushes stdout, so the console sink needs no extra step.

#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <atomic>
#include <format>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <utility>
#include <exception>
#include <functional>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/sink.hpp>
#include <krumb/core/level.hpp>
#include <krumb/core/format_string.hpp>

// ERROR HANDLER -----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    using error_handler = std::function<void(std::string_view)>;
}

// LOGGER STATE ------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb::detail {
    // One destination of a logger, with its own minimum level.
    struct sink_entry {
        std::shared_ptr<sink> target;
        level threshold;
    };

    // The data shared by every handle of a logger. Internal: use krumb::logger and krumb::logger_builder.
    struct logger_state {
        logger_state() = default;
        ~logger_state();

        logger_state(const logger_state&) = delete;
        logger_state& operator = (const logger_state&) = delete;

        void write(level lvl, const std::source_location& location, std::string_view message) noexcept;
        void flush() noexcept;
        void flush_sink(std::size_t index) noexcept;
        
        void report_error(std::string_view context, std::string_view detail) noexcept;
        void report_sink_error(std::size_t index, std::string_view operation, std::string_view detail) noexcept;

        void deliver(std::string_view description);

        std::atomic<level> active_level{level::info};
        std::atomic<level> flush_level{level::error};

        level min_threshold = level::trace;
        
        std::vector<sink_entry> sinks;
        std::mutex handler_mutex;
        error_handler handler;
    };
}

// LOGGER ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Formats messages, filters them by level, and hands them to a sink.
    // A logger is a cheap handle: copies share the same level, sinks and error handler.
    class logger {
        public:
        logger();

        explicit logger(std::shared_ptr<sink> target);

        logger(const logger&) = default;
        logger& operator = (const logger&) = default;
        ~logger() = default;

        void set_level(level lvl) noexcept {
            state_->active_level.store(lvl, std::memory_order_relaxed);
        }

        level get_level() const noexcept {
            return state_->active_level.load(std::memory_order_relaxed);
        }

        bool should_log(level lvl) const noexcept {
            return lvl != level::off && lvl >= get_level() && lvl >= state_->min_threshold;
        }

        void set_flush_level(level lvl) noexcept {
            state_->flush_level.store(lvl, std::memory_order_relaxed);
        }

        level get_flush_level() const noexcept {
            return state_->flush_level.load(std::memory_order_relaxed);
        }

        void flush() noexcept {
            state_->flush();
        }

        void set_error_handler(error_handler handler);

        // Filters first, so a dropped message costs no formatting.
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
                state_->report_error("message formatting failed", ex.what());
                return;
            }
            catch (...) {
                state_->report_error("message formatting failed", "unknown exception");
                return;
            }

            state_->write(lvl, fmt.location, message);
        }

        private:
        friend class logger_builder;
        explicit logger(std::shared_ptr<detail::logger_state> state) noexcept;

        std::shared_ptr<detail::logger_state> state_;
    };
}

// DEFAULT LOGGER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger default_logger();
    
    void set_default_logger(logger replacement);
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

