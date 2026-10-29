#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <atomic>
#include <format>
#include <string>
#include <utility>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/level.hpp>
#include <krumb/core/format_string.hpp>
#include <krumb/sinks/console_sink.hpp>
#include <krumb/formatters/text_formatter.hpp>

// LOGGER ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    class logger {
        public:
            logger() = default;
            logger(const logger&) = delete;
            logger& operator = (const logger&) = delete;

            void set_level(level lvl) noexcept {
                level_.store(lvl, std::memory_order_relaxed);
            }

            level get_level() const noexcept {
                return level_.load(std::memory_order_relaxed);
            }

            bool should_log(level lvl) const noexcept {
                return lvl != level::off && lvl >= get_level();
            }

            template <typename... Args>
            void log(level lvl, location_format_string<Args...> fmt, Args&&... args) {
                if (!should_log(lvl)) {
                    return;
                }

                const std::string message = std::format(fmt.fmt, std::forward<Args>(args)...);
                write(lvl, fmt.location, message);
            }

        private:
            void write(level lvl, const std::source_location& location, std::string_view message);

            std::atomic<level> level_{level::info};
            std::mutex mutex_;
            text_formatter formatter_;
            console_sink sink_;
            std::string buffer_;
    };
}

// DEFAULT LOGGER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger& default_logger();
    void set_level(level lvl) noexcept;
    level get_level() noexcept;
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
